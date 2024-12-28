// server.cpp - SSL/TLS enabled server
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <filesystem>
#include <csignal>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/md5.h>

#define PORT 8080
#define BUFFER_SIZE 1024

using namespace std;
namespace fs = std::filesystem;

string dir = "./cloud/"; // Directory for shared files
int sockfd;
SSL_CTX *ssl_ctx;

string calculate_md5(const string &filename)
{
    ifstream file(filename, ios::binary);
    if (!file.is_open())
        return "";
    MD5_CTX md5_ctx;
    MD5_Init(&md5_ctx);
    char buffer[BUFFER_SIZE];
    while (file.read(buffer, sizeof(buffer)))
    {
        MD5_Update(&md5_ctx, buffer, file.gcount());
    }
    unsigned char result[MD5_DIGEST_LENGTH];
    MD5_Final(result, &md5_ctx);
    stringstream ss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; ++i)
        ss << hex << setw(2) << setfill('0') << (int)result[i];
    return ss.str();
}

void init_openssl()
{
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl()
{
    EVP_cleanup();
}

SSL_CTX *create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();
    ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        cerr << "Unable to create SSL context" << endl;
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_context(SSL_CTX *ctx)
{
    // Set the key and cert
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

void handle_client(SSL *ssl, struct sockaddr_in client_addr);
void send_file_list(SSL *ssl);
void send_file(SSL *ssl, const string &filename);
void receive_file(SSL *ssl, const string &filename);

void handle_signal(int signum)
{
    cout << "\nShutting down server...\n";
    close(sockfd);
    SSL_CTX_free(ssl_ctx);
    cleanup_openssl();
    exit(0);
}

int main()
{
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    init_openssl();
    ssl_ctx = create_context();
    configure_context(ssl_ctx);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        cerr << "Socket creation failed!\n";
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        cerr << "Binding failed!\n";
        return -1;
    }

    signal(SIGINT, handle_signal);

    if (listen(sockfd, 10) < 0)
    {
        cerr << "Listening failed!\n";
        return -1;
    }

    cout << "Server is listening on port " << PORT << "...\n";

    while (true)
    {
        int new_sock = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (new_sock < 0)
        {
            cerr << "Connection acceptance failed!\n";
            continue;
        }

        SSL *ssl = SSL_new(ssl_ctx);
        SSL_set_fd(ssl, new_sock);

        if (SSL_accept(ssl) <= 0)
        {
            ERR_print_errors_fp(stderr);
            close(new_sock);
            continue;
        }

        // cout << "New connection accepted from client.\n";
        cout << "New connection from IP: " << inet_ntoa(client_addr.sin_addr)
             << ", Port: " << ntohs(client_addr.sin_port) << "\n";

        thread client_thread([ssl, client_addr]()
                             {
            handle_client(ssl, client_addr);
            SSL_free(ssl); });
        client_thread.detach();
    }

    close(sockfd);
    SSL_CTX_free(ssl_ctx);
    cleanup_openssl();
    return 0;
}

void handle_client(SSL *ssl, struct sockaddr_in client_addr)
{
    char choice;
    while (SSL_read(ssl, &choice, sizeof(choice)) > 0)
    {
        switch (choice)
        {
        case '1':
            cout << "Client from IP: " << inet_ntoa(client_addr.sin_addr)
                 << " requested file list.\n";
            send_file_list(ssl);
            break;
        case '2':
        {
            char filename[BUFFER_SIZE];
            SSL_read(ssl, filename, BUFFER_SIZE);
            cout << "Client from IP: " << inet_ntoa(client_addr.sin_addr)
                 << " requested to download file: " << filename << "\n";
            send_file(ssl, dir + filename);
            break;
        }
        case '3':
        {
            char filename[BUFFER_SIZE];
            SSL_read(ssl, filename, BUFFER_SIZE);
            cout << "Client from IP: " << inet_ntoa(client_addr.sin_addr)
                 << " is uploading file: " << filename << "\n";
            receive_file(ssl, dir + filename);
            break;
        }
        case '4':
            cout << "Client disconnected from IP: " << inet_ntoa(client_addr.sin_addr)
                 << ", Port: " << ntohs(client_addr.sin_port) << "\n";
            SSL_shutdown(ssl);
            return;
        default:
            cerr << "Invalid choice from client!\n";
        }
    }
    SSL_shutdown(ssl);
    cout << "Client disconnected from IP: " << inet_ntoa(client_addr.sin_addr)
         << ", Port: " << ntohs(client_addr.sin_port) << "\n";
}

void send_file_list(SSL *ssl)
{
    string file_list;
    for (const auto &entry : fs::directory_iterator(dir))
    {
        file_list += entry.path().filename().string() + "\n";
    }

    size_t list_size = file_list.size();
    SSL_write(ssl, &list_size, sizeof(list_size));
    SSL_write(ssl, file_list.c_str(), list_size);

    cout << "Sent file list to client. Total bytes sent: " << list_size << "\n";
}

void send_file(SSL *ssl, const string &filename)
{
    ifstream file(filename, ios::binary);
    if (!file.is_open())
    {
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
    while (file.read(buffer, sizeof(buffer)))
    {
        SSL_write(ssl, buffer, file.gcount());
    }
    if (file.gcount() > 0)
    {
        SSL_write(ssl, buffer, file.gcount());
    }

    string checksum = calculate_md5(filename);
    SSL_write(ssl, checksum.c_str(), checksum.size() + 1);

    // Thêm in checksum trên server
    cout << "File sent: " << filename << " (size: " << file_size
         << " bytes, checksum: " << checksum << ")\n";
    file.close();
}

void receive_file(SSL *ssl, const string &filename)
{
    ofstream file(filename, ios::binary);
    if (!file.is_open())
    {
        cerr << "Failed to open file for writing: " << filename << "\n";
        return;
    }

    size_t file_size;
    SSL_read(ssl, &file_size, sizeof(file_size));

    char buffer[BUFFER_SIZE];
    size_t total_bytes_received = 0;
    while (file_size > 0)
    {
        ssize_t bytes_received = SSL_read(ssl, buffer, std::min<size_t>(BUFFER_SIZE, file_size));
        if (bytes_received <= 0)
            break;
        file.write(buffer, bytes_received);
        file_size -= bytes_received;
        total_bytes_received += bytes_received;
    }
    file.close();

    // Nhận checksum từ client
    char client_checksum[MD5_DIGEST_LENGTH * 2 + 1] = {0};
    SSL_read(ssl, client_checksum, sizeof(client_checksum));

    // Tính checksum của file vừa lưu
    string server_checksum = calculate_md5(filename);

    // So sánh checksum
    if (server_checksum == string(client_checksum))
    {
        cout << "File received successfully. Checksum verified: " << server_checksum << "\n";
    }
    else
    {
        cerr << "Checksum mismatch for file: " << filename << "\n";
        cerr << "Expected: " << client_checksum << "\n";
        cerr << "Received: " << server_checksum << "\n";
    }

    cout << "File received: " << filename << " (size: " << total_bytes_received << " bytes)\n";
}

void delete_file(SSL *ssl)
{
    char filename[BUFFER_SIZE];
    SSL_read(ssl, filename, BUFFER_SIZE);
    string filepath = dir + filename;

    if (fs::exists(filepath) && fs::is_regular_file(filepath))
    {
        fs::remove(filepath);
        char response = '1';
        SSL_write(ssl, &response, sizeof(response));
        // Thông báo file bị xóa
        cout << "File deleted: " << filepath << "\n";
    }
    else
    {
        char response = '0';
        SSL_write(ssl, &response, sizeof(response));
        // Thông báo file không tồn tại
        cout << "Failed to delete file (not found): " << filepath << "\n";
    }
}