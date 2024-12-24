#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <termios.h>

#define PORT 8080
#define BUFFER_SIZE 1024

using namespace std;

string IP_SERVER = "127.0.0.1"; // Default server IP
int sock;

void list_files();
void download_file();
void upload_file();
void exit_client();
void handle_signal(int signum);

int main() {
    struct sockaddr_in server_addr;

    cout << "Enter server IP (default " << IP_SERVER << "): ";
    string input_ip;
    getline(cin, input_ip);
    if (!input_ip.empty()) {
        IP_SERVER = input_ip;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Socket creation failed!\n";
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, IP_SERVER.c_str(), &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Connection to server failed!\n";
        return -1;
    }

    signal(SIGINT, handle_signal);

    cout << "Connected to server.\n";

    while (true) {
        cout << "\n1. List files\n2. Download file\n3. Upload file\n4. Exit\nChoose: ";
        char choice;
        cin >> choice;
        cin.ignore();

        switch (choice) {
        case '1':
            list_files();
            break;
        case '2':
            download_file();
            break;
        case '3':
            upload_file();
            break;
        case '4':
            exit_client();
            return 0;
        default:
            cout << "Invalid choice!\n";
        }
    }
}

void list_files() {
    char choice = '1';
    send(sock, &choice, sizeof(choice), 0);

    char buffer[BUFFER_SIZE];
    while (true) {
        ssize_t bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0 || string(buffer, bytes_received) == "EOF") {
            break;
        }
        cout << string(buffer, bytes_received);
    }
    cout << "File list received successfully.\n";
}

void download_file() {
    cout << "Enter filename to download: ";
    string filename;
    cin >> filename;

    char choice = '2';
    send(sock, &choice, sizeof(choice), 0);
    send(sock, filename.c_str(), filename.size() + 1, 0);

    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Failed to create file!\n";
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t total_bytes_received = 0;
    while (true) {
        ssize_t bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0 || string(buffer, bytes_received) == "EOF") {
            break;
        }
        file.write(buffer, bytes_received);
        total_bytes_received += bytes_received;
    }
    file.close();
    cout << "File downloaded successfully. Total bytes received: " << total_bytes_received << "\n";
}

void upload_file() {
    cout << "Enter filename to upload: ";
    string filename;
    cin >> filename;

    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "File not found!\n";
        return;
    }

    char choice = '3';
    send(sock, &choice, sizeof(choice), 0);
    send(sock, filename.c_str(), filename.size() + 1, 0);

    char buffer[BUFFER_SIZE];
    size_t total_bytes_sent = 0;
    while (file.read(buffer, sizeof(buffer))) {
        ssize_t bytes_sent = send(sock, buffer, file.gcount(), 0);
        total_bytes_sent += bytes_sent;
    }
    send(sock, "EOF", 3, 0);
    file.close();
    cout << "File uploaded successfully. Total bytes sent: " << total_bytes_sent << "\n";
}

void exit_client() {
    char choice = '4';
    send(sock, &choice, sizeof(choice), 0);
    close(sock);
    cout << "Disconnected from server.\n";
}

void handle_signal(int signum) {
    exit_client();
    exit(0);
}
