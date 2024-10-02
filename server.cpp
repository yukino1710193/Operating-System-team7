#include <iostream>     // nhập xuất ra terminal
#include <sys/socket.h> // lập trình socket
#include <netinet/in.h> // IPv4 , Ipv6
#include <unistd.h>     // thao tác với file hệ thống ( file descriptor )
#include <fstream>      // thao tác với file
#include <cstring>      // xử lý chuỗi
#include <thread>       // luồng
#include <sys/types.h>  // các type hệ thống như process_id ,..
#include <dirent.h>     // thao tác với thư mục
#include <arpa/inet.h>  // tương tác với địa chỉ mạng
#include <csignal>      // xử lý tín hiệu ngắt
#include <atomic>       // Biến an toàn cho luồng
#include <termios.h> // Để làm việc với các thuộc tính terminal

using namespace std;

#define PORT 8080
#define BUFFER_SIZE 1024

int sockfd;
string FOLDER = "." ;
// Declare
void handle_client(int new_sock);
void send_file_list(int sock);
void send_file(int sock, const string &filename);
void receive_file(int sock, const string &filename);
void reset_terminal_mode();
int main()
{   
    // declare
    int new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Tạo socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0) // Tạo socket rồi kiểm tra đã tạo thành công hay chưa qua giá trị trả về
    {
        cerr << "Socket lỗi khởi tạo" << endl; // in ra thông báo lỗi
        return -1;                             // trả về lỗi , thoát chương trình
    }

    memset(&server_addr, 0, sizeof(server_addr)); // Reset giá trị của server_addr về 0 trước khi gán các giá trị cho
                                                  // các properties của nó
    // Đặt địa chỉ và cổng cho server
    server_addr.sin_family = AF_INET;         // Version IP : IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // Đặt địa chỉ IP của server : any = 0.0.0.0 lắng nghe từ mọi IP của server có
    server_addr.sin_port = htons(PORT);       // Chọn port cho dịch vụ

    // hàm htons exchange địa chỉ port từ format của host sang network

    // Gắn socket với cổng và địa chỉ
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) // bind socket vào cặp IP:port đã khai báo
    {
        cerr << "Bind lỗi" << endl; // in ra lỗi nếu bind thất bại
        return -1;                  // ngừng chương trình , trả về lỗi
    }

    // Đăng ký tín hiệu SIGINT (Ctrl+C) để gọi signal_handler
    signal(SIGINT, [](int signum) -> void
           {
               cout << "\nNhận tín hiệu ngắt (Ctrl+C), đóng socket...\n";
               close(sockfd);
               reset_terminal_mode(); // Khôi phục trạng thái terminal
               exit(0);
           });

    // Lắng nghe kết nối từ client
    if (listen(sockfd, 10) < 0) // server lắng nghe trên socket đã tạo và bind ; giới hạn hàng đợi là 10 kết nối
    {
        cerr << "Listen lỗi" << endl; // thông báo lỗi
        return -1;
    }

    cout << "Server đang chờ kết nối..." << endl; // thông báo server đang trong quá trình listen
    // Xử lý kết nối từ client
    while (true)
    {
        // chấp nhận kết nối
        new_sock = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);

        // kiểm lỗi báo lỗi
        if (new_sock < 0)
        {
            perror("Chấp nhận thất bại");
            continue;
        }

        // tách luồng ra để xử lý
        thread client_thread(handle_client, new_sock);
        client_thread.detach(); // thread chạy xong sẽ tự giải phóng tài nguyên , server tiếp tục lắng nghe
    }

    close(sockfd); // Đóng socket
    reset_terminal_mode(); // Khôi phục trạng thái terminal
    return 0;
}

void handle_client(int new_sock)
{
    char choice; // lưu lựa chọn được gửi từ client
    while (true)
    {
        recv(new_sock, &choice, sizeof(choice), 0); // nhận data được lắng nghe từ socket , đặt vào biến choice
        
        // Xử lý với từng choice
        switch (choice)
        {
        case '1':
            send_file_list(new_sock);   // gủi danh sách file
            break;

        case '2':
        {
            char filename[BUFFER_SIZE];
            recv(new_sock, filename, sizeof(filename), 0);      // đọc xem client yêu cầu file nào , độ dài như thế nào
            send_file(new_sock, filename);                      // Gửi file mà client yêu cầu download
            break;
        }

        case '3':
        {
            char filename[BUFFER_SIZE];
            recv(new_sock, filename, sizeof(filename), 0);      // đọc xem client yêu cầu upload file nào , độ dài như thế nào
            receive_file(new_sock, filename);                   // nhận file client gửi
            break;
        }

        case '4':
            break;                                              // thoát khỏi kết nối client-server này

        default:
            cout << "Invalid choice received: " << choice << endl;
            break;
        }
    }

    close(new_sock);                                            // đóng socket
}

void send_file_list(int sock) {
    FILE* fp = popen("ls", "r"); // Thực thi lệnh ls
    if (fp == nullptr) {
        perror("Failed to run ls command");
        return;
    }

    string file_list;
    char buffer[BUFFER_SIZE];

    // Đọc dữ liệu từ lệnh ls và lưu vào chuỗi
    while (fgets(buffer, sizeof(buffer), fp) != nullptr) {
        file_list += buffer; // Nối từng dòng vào file_list
    }

    fclose(fp); // Đóng pipe

    // In danh sách file trước khi gửi
    cout << "Danh sách file trên server:\n" << file_list << endl;

    // Gửi toàn bộ danh sách file
    ssize_t bytes_sent = send(sock, file_list.c_str(), file_list.size(), 0);
    if (bytes_sent == -1) {
        perror("Send failed");
    } else {
        cout << "Đã gửi " << bytes_sent << " byte tới client.\n";
    }

    // Gửi tín hiệu EOF riêng biệt sau khi gửi xong danh sách file
    const char* eof_signal = "EOF";
    ssize_t bytes_sent_eof = send(sock, eof_signal, strlen(eof_signal), 0);
    if (bytes_sent_eof == -1) {
        perror("Send EOF failed");
    } else {
        cout << "Đã gửi tín hiệu EOF tới client.\n";
    }
}


void send_file(int sock, const string &filename)
{
    ifstream file(filename, ios::binary);
    if (!file.is_open())
    {
        perror("File not found");
        return;
    }

    char buffer[BUFFER_SIZE];
    while (file.read(buffer, sizeof(buffer)))
    {
        send(sock, buffer, file.gcount(), 0);
    }

    send(sock, buffer, file.gcount(), 0); // Send the remaining part of the file
    file.close();
    cout << "File " << filename << " sent successfully.\n";
}

void receive_file(int sock, const string &filename)
{
    ofstream file(filename, ios::binary);
    if (!file.is_open())
    {
        perror("Error opening file for writing");
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    while ((bytes_received = recv(sock, buffer, sizeof(buffer), 0)) > 0)
    {
        file.write(buffer, bytes_received);
    }

    file.close();
    cout << "File " << filename << " received successfully.\n";
}

// Hàm khôi phục terminal
void reset_terminal_mode() {
    struct termios t;
    tcgetattr(STDIN_FILENO, &t); // Lấy thông tin cài đặt terminal hiện tại
    t.c_lflag |= ICANON | ECHO; // Bật lại chế độ canonical và echo
    tcsetattr(STDIN_FILENO, TCSANOW, &t); // Áp dụng thay đổi ngay lập tức
}