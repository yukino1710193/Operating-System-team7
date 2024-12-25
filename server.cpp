// server.cpp - SSL/TLS enabled server
#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <filesystem>
#include <csignal>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 8080
#define BUFFER_SIZE 1024

using namespace std;
namespace fs = std::filesystem;

string dir = "./cloud/"; // Directory for shared files
int sockfd;
SSL_CTX *ssl_ctx;

void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
    EVP_cleanup();
}

SSL_CTX *create_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        cerr << "Unable to create SSL context" << endl;
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_context(SSL_CTX *ctx) {
    // Set the key and cert
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

void handle_client(SSL *ssl);
void send_file_list(SSL *ssl);
void send_file(SSL *ssl, const string &filename);
void receive_file(SSL *ssl, const string &filename);

void handle_signal(int signum) {
    cout << "\nShutting down server...\n";
    close(sockfd);
    SSL_CTX_free(ssl_ctx);
    cleanup_openssl();
    exit(0);
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    init_openssl();
    ssl_ctx = create_context();
    configure_context(ssl_ctx);

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

        SSL *ssl = SSL_new(ssl_ctx);
        SSL_set_fd(ssl, new_sock);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
            close(new_sock);
            continue;
        }

        cout << "New connection accepted from client.\n";

        thread client_thread([ssl]() {
            handle_client(ssl);
            SSL_free(ssl);
        });
        client_thread.detach();
    }

    close(sockfd);
    SSL_CTX_free(ssl_ctx);
    cleanup_openssl();
    return 0;
}

void handle_client(SSL *ssl) {
    char choice;
    while (SSL_read(ssl, &choice, sizeof(choice)) > 0) {
        switch (choice) {
        case '1':
            cout << "Client requested file list.\n";
            send_file_list(ssl);
            break;
        case '2': {
            char filename[BUFFER_SIZE];
            SSL_read(ssl, filename, BUFFER_SIZE);
            cout << "Client requested to download file: " << filename << "\n";
            send_file(ssl, dir + filename);
            break;
        }
        case '3': {
            char filename[BUFFER_SIZE];
            SSL_read(ssl, filename, BUFFER_SIZE);
            cout << "Client is uploading file: " << filename << "\n";
            receive_file(ssl, dir + filename);
            break;
        }
        case '4':
            cout << "Client disconnected.\n";
            SSL_shutdown(ssl);
            return;
        default:
            cerr << "Invalid choice from client!\n";
        }
    }
    SSL_shutdown(ssl);
}

void send_file_list(SSL *ssl) {
    string file_list;
    for (const auto &entry : fs::directory_iterator(dir)) {
        file_list += entry.path().filename().string() + "\n";
    }

    size_t list_size = file_list.size();
    SSL_write(ssl, &list_size, sizeof(list_size));
    SSL_write(ssl, file_list.c_str(), list_size);

    cout << "Sent file list to client. Total bytes sent: " << list_size << "\n";
}

void send_file(SSL *ssl, const string &filename) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "File not found: " << filename << "\n";
        size_t error_size = 0;
        SSL_write(ssl, &error_size, sizeof(error_size));
        return;
    }

    file.seekg(0, ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, ios::beg);

    SSL_write(ssl, &file_size, sizeof(file_size));

    char buffer[BUFFER_SIZE];
    size_t total_bytes_sent = 0;

    while (file.read(buffer, sizeof(buffer))) {
        total_bytes_sent += SSL_write(ssl, buffer, file.gcount());
    }

    if (file.gcount() > 0) {
        total_bytes_sent += SSL_write(ssl, buffer, file.gcount());
    }

    cout << "File " << filename << " sent successfully. Total bytes sent: " << total_bytes_sent << "\n";
    file.close();
}

void receive_file(SSL *ssl, const string &filename) {
    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Failed to open file for writing: " << filename << "\n";
        return;
    }

    size_t file_size;
    SSL_read(ssl, &file_size, sizeof(file_size));

    char buffer[BUFFER_SIZE];
    size_t total_bytes_received = 0;

    while (total_bytes_received < file_size) {
        ssize_t bytes_received = SSL_read(ssl, buffer, std::min(static_cast<size_t>(BUFFER_SIZE), file_size - total_bytes_received));
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
