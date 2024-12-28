// client.cpp - SSL/TLS enabled client
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/md5.h>

#define PORT 8080
#define BUFFER_SIZE 1024

using namespace std;

string IP_SERVER = "127.0.0.1"; // Default server IP
int sock;
SSL_CTX *ssl_ctx;
SSL *ssl;

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

    method = SSLv23_client_method();
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
    // Load the CA certificate (server.crt)
    if (SSL_CTX_load_verify_locations(ctx, "server.crt", NULL) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    // Require server certificate verification
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    SSL_CTX_set_verify_depth(ctx, 1);
}

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

void list_files();
void download_file();
void upload_file();
void exit_client();
void handle_signal(int signum);

int main()
{
    struct sockaddr_in server_addr;

    init_openssl();
    ssl_ctx = create_context();
    configure_context(ssl_ctx);

    cout << "Enter server IP (default " << IP_SERVER << "): ";
    string input_ip;
    getline(cin, input_ip);
    if (!input_ip.empty())
    {
        IP_SERVER = input_ip;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        cerr << "Socket creation failed!\n";
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, IP_SERVER.c_str(), &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        cerr << "Connection to server failed!\n";
        return -1;
    }

    ssl = SSL_new(ssl_ctx);
    SSL_set_fd(ssl, sock);

    if (SSL_connect(ssl) <= 0)
    {
        ERR_print_errors_fp(stderr);
        return -1;
    }

    signal(SIGINT, handle_signal);

    cout << "Connected to server securely.\n";

    while (true)
    {
        cout << "\n1. List files\n2. Download file\n3. Upload file\n4. Exit\nChoose: ";
        char choice;
        cin >> choice;
        cin.ignore();

        switch (choice)
        {
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

void list_files()
{
    char choice = '1';
    SSL_write(ssl, &choice, sizeof(choice));

    size_t list_size;
    SSL_read(ssl, &list_size, sizeof(list_size)); // Receive size of the list

    char buffer[BUFFER_SIZE];
    size_t total_bytes_received = 0;

    while (total_bytes_received < list_size)
    {
        ssize_t bytes_received = SSL_read(ssl, buffer, std::min(static_cast<size_t>(BUFFER_SIZE), list_size - total_bytes_received));
        if (bytes_received <= 0)
        {
            cerr << "Error receiving file list.\n";
            return;
        }
        cout << string(buffer, bytes_received);
        total_bytes_received += bytes_received;
    }
    cout << "File list received successfully.\n";
}

void download_file()
{
    cout << "Enter filename to download: ";
    string filename;
    cin >> filename;

    char choice = '2';
    SSL_write(ssl, &choice, sizeof(choice));
    SSL_write(ssl, filename.c_str(), filename.size() + 1);

    size_t file_size;
    SSL_read(ssl, &file_size, sizeof(file_size));

    if (file_size == 0)
    {
        cerr << "Error: File not found on server.\n";
        return;
    }

    ofstream file(filename, ios::binary);
    if (!file.is_open())
    {
        cerr << "Failed to create file!\n";
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t total_bytes_received = 0;
    while (total_bytes_received < file_size)
    {
        ssize_t bytes_received = SSL_read(ssl, buffer, std::min(static_cast<size_t>(BUFFER_SIZE), file_size - total_bytes_received));
        if (bytes_received <= 0)
        {
            cerr << "Error receiving file data.\n";
            break;
        }
        file.write(buffer, bytes_received);
        total_bytes_received += bytes_received;
    }
    file.close();

    // Nhận checksum từ server
    char server_checksum[MD5_DIGEST_LENGTH * 2 + 1] = {0};
    SSL_read(ssl, server_checksum, sizeof(server_checksum));
    string local_checksum = calculate_md5(filename);

    cout << "File downloaded successfully. Total bytes received: " << total_bytes_received << "\n";

    // So sánh và in checksum
    cout << "Checksum from server: " << server_checksum << "\n";
    cout << "Checksum calculated locally: " << local_checksum << "\n";

    if (local_checksum == string(server_checksum))
    {
        cout << "Checksum matched! File integrity verified.\n";
    }
    else
    {
        cerr << "Checksum mismatch! File may be corrupted.\n";
    }
}

void upload_file()
{
    cout << "Enter filename to upload: ";
    string filename;
    cin >> filename;

    ifstream file(filename, ios::binary);
    if (!file.is_open())
    {
        cerr << "File not found!\n";
        return;
    }

    char choice = '3';
    SSL_write(ssl, &choice, sizeof(choice));
    SSL_write(ssl, filename.c_str(), filename.size() + 1);

    file.seekg(0, ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, ios::beg);
    SSL_write(ssl, &file_size, sizeof(file_size)); // Send size of the file

    char buffer[BUFFER_SIZE];
    size_t total_bytes_sent = 0;
    while (file.read(buffer, sizeof(buffer)))
    {
        total_bytes_sent += SSL_write(ssl, buffer, file.gcount());
    }

    if (file.gcount() > 0)
    {
        total_bytes_sent += SSL_write(ssl, buffer, file.gcount());
    }

    // Gửi checksum của file
    string checksum = calculate_md5(filename);
    SSL_write(ssl, checksum.c_str(), checksum.size() + 1);

    file.close();
    cout << "File uploaded successfully. Total bytes sent: " << total_bytes_sent << "\n";
}

void exit_client()
{
    char choice = '4';
    SSL_write(ssl, &choice, sizeof(choice));
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sock);
    SSL_CTX_free(ssl_ctx);
    cleanup_openssl();
    cout << "Disconnected from server.\n";
}

void handle_signal(int signum)
{
    exit_client();
    exit(0);
}
