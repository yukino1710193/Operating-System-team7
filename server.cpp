#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <filesystem>
#include <csignal>
#include <termios.h>

#define PORT 8080
#define BUFFER_SIZE 1024

using namespace std;
namespace fs = std::filesystem;

string dir = "./cloud/"; // Directory for shared files
int sockfd;

void handle_client(int new_sock);
void send_file_list(int sock);
void send_file(int sock, const string &filename);
void receive_file(int sock, const string &filename);
void reset_terminal_mode();

void handle_signal(int signum) {
    cout << "\nShutting down server...\n";
    close(sockfd);
    exit(0);
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        cerr << "Socket creation failed!\n";
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Binding failed!\n";
        return -1;
    }

    signal(SIGINT, handle_signal);

    if (listen(sockfd, 10) < 0) {
        cerr << "Listening failed!\n";
        return -1;
    }

    cout << "Server is listening on port " << PORT << "...\n";

    while (true) {
        int new_sock = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (new_sock < 0) {
            cerr << "Connection acceptance failed!\n";
            continue;
        }

        cout << "New connection accepted from client.\n";

        thread client_thread(handle_client, new_sock);
        client_thread.detach();
    }

    close(sockfd);
    return 0;
}

void handle_client(int new_sock) {
    char choice;
    while (recv(new_sock, &choice, sizeof(choice), 0) > 0) {
        switch (choice) {
        case '1':
            cout << "Client requested file list.\n";
            send_file_list(new_sock);
            break;
        case '2': {
            char filename[BUFFER_SIZE];
            recv(new_sock, filename, BUFFER_SIZE, 0);
            cout << "Client requested to download file: " << filename << "\n";
            send_file(new_sock, dir + filename);
            break;
        }
        case '3': {
            char filename[BUFFER_SIZE];
            recv(new_sock, filename, BUFFER_SIZE, 0);
            cout << "Client is uploading file: " << filename << "\n";
            receive_file(new_sock, dir + filename);
            break;
        }
        case '4':
            cout << "Client disconnected.\n";
            close(new_sock);
            return;
        default:
            cerr << "Invalid choice from client!\n";
        }
    }
    close(new_sock);
}

void send_file_list(int sock) {
    string file_list;
    for (const auto &entry : fs::directory_iterator(dir)) {
        file_list += entry.path().filename().string() + "\n";
    }

    size_t list_size = file_list.size();
    send(sock, &list_size, sizeof(list_size), 0); // Send size of the list
    send(sock, file_list.c_str(), list_size, 0);

    cout << "Sent file list to client. Total bytes sent: " << list_size << "\n";
}

void send_file(int sock, const string &filename) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "File not found: " << filename << "\n";
        size_t error_size = 0;
        send(sock, &error_size, sizeof(error_size), 0);
        return;
    }

    file.seekg(0, ios::end);
    size_t file_size = file.tellg();
    cout << "Calculated file size: " << file_size << " bytes" << endl;
    file.seekg(0, ios::beg);

    send(sock, &file_size, sizeof(file_size), 0); // Send size of the file
    cout << "Sent file size: " << file_size << " bytes to server" << endl;

    char buffer[BUFFER_SIZE];
    size_t total_bytes_sent = 0;

    while (file.read(buffer, sizeof(buffer))) {
        ssize_t bytes_sent = send(sock, buffer, file.gcount(), 0);
        total_bytes_sent += bytes_sent;
    }

    if (file.gcount() > 0) {
        ssize_t bytes_sent = send(sock, buffer, file.gcount(), 0);
        total_bytes_sent += bytes_sent;
    }

    cout << "File " << filename << " sent successfully. Total bytes sent: " << total_bytes_sent << "\n";
    file.close();
}

void receive_file(int sock, const string &filename) {
    cout << "Starting to receive file upload request..." << endl;
    cout << "File name received: " << filename << "\n";
    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Failed to open file for writing: " << filename << "\n";
        return;
    }

    size_t file_size;
    if (recv(sock, &file_size, sizeof(file_size), 0) > 0) {
        cout << "File size received successfully: " << file_size << " bytes" << endl;
    } else {
        cerr << "Failed to receive file size.";
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    size_t total_bytes_received = 0;

    while (total_bytes_received < file_size) {
        bytes_received = recv(sock, buffer, std::min(static_cast<size_t>(BUFFER_SIZE), file_size - total_bytes_received), 0);
        if (bytes_received <= 0) {
            cerr << "Error receiving file data.\n";
            break;
        }
        file.write(buffer, bytes_received);
        total_bytes_received += bytes_received;
    }

    cout << "File " << filename << " received successfully. Total bytes received: " << total_bytes_received << "\n";
    file.close();
}