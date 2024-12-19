// Client.cpp
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>      // memset
#include <sys/socket.h> // socket, connect, send, recv
#include <arpa/inet.h>  // inet_pton
#include <unistd.h>     // close
#include <sys/types.h>
#include <netinet/in.h>
#include <vector>
#include <csignal>      // xử lý tín hiệu ngắt
#include <termios.h> // Để làm việc với các thuộc tính terminal

#define PORT 8080
#define BUFFER_SIZE 1024

using namespace std;

const char* SERVER_IP = "127.0.0.1";
int sock=0;

void list_files(int sock);
void download_file(int sock);
void upload_file(int sock);
void exit_client(int sock);
void reset_terminal_mode();
bool send_all(int sock, const char* data, size_t length);

int main() {
    struct sockaddr_in serv_addr;

    // Tạo socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << "Socket tạo thất bại.\n";
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Chuyển địa chỉ IPv4 từ text sang binary form
    if(inet_pton(AF_INET, SERVER_IP , &serv_addr.sin_addr) <= 0) {
        cerr << "Invalid address/ Address not supported.\n";
        return -1;
    }

    signal(SIGINT, [](int signum) -> void
           {
               cout << "\nNhận tín hiệu ngắt (Ctrl+C), đóng socket...\n";
               exit_client(sock);
               close(sock);
               reset_terminal_mode(); // Khôi phục trạng thái terminal
               exit(0);
           });

    // Kết nối đến server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        cerr << "Kết nối đến server thất bại.\n";
        return -1;
    }

    cout << "Kết nối đến server thành công.\n";

    while (true) {
        cout << "\n--- Menu ---\n";
        cout << "1. Xem danh sách file trên server\n";
        cout << "2. Tải xuống file từ server\n";
        cout << "3. Tải lên file lên server\n";
        cout << "4. Thoát\n";
        cout << "Chọn lựa: ";

        string input;
        getline(cin, input);

        if (input.empty()) {
            cout << "Vui lòng chọn một lựa chọn hợp lệ.\n";
            continue;
        }

        char choice = input[0];

        switch (choice) {
            case '1':
                list_files(sock);
                break;
            case '2':
                download_file(sock);
                break;
            case '3':
                upload_file(sock);
                break;
            case '4':
                exit_client(sock);
                close(sock);
                return 0;
            default:
                cout << "Lựa chọn không hợp lệ. Vui lòng thử lại.\n";
        }
    }

    // Đóng socket (nếu thoát vòng lặp mà chưa đóng)
    close(sock);
    return 0;
}


// Hàm gửi dữ liệu đến server, đảm bảo gửi hết dữ liệu
bool send_all(int sock, const char* data, size_t length) {
    size_t total_sent = 0;

    // Gửi độ dài dữ liệu trước
    ssize_t sent = send(sock, &length, sizeof(length), 0);
    if (sent == -1) {
        perror("send length");
        return false;
    }

    // Gửi dữ liệu thực tế
    while (total_sent < length) {
        sent = send(sock, data + total_sent, length - total_sent, 0);
        if (sent == -1) {
            perror("send data");
            return false;
        }
        total_sent += sent;
    }

    return true;
}

// Hàm nhận dữ liệu từ server, đảm bảo nhận đủ dữ liệu
bool recv_all(int sock, char* buffer, size_t length) {
    size_t total_received = 0;
    while (total_received < length) {
        ssize_t received = recv(sock, buffer + total_received, length - total_received, 0);
        if (received <= 0) {
            return false;
        }
        total_received += received;
    }
    return true;
}

// Hàm nhận danh sách file từ server
void list_files(int sock) {
    // Gửi lựa chọn '1' đến server
    char choice = '1';
    if (!send_all(sock, &choice, sizeof(choice))) {
        cerr << "Gửi lựa chọn thất bại.\n";
        return;
    }

    // Nhận danh sách file từ server
    cout << "Danh sách file và thư mục trên server:\n";

    while (true) {
        char filename[BUFFER_SIZE]; // Bộ đệm để nhận tên file
        ssize_t received = recv(sock, filename, sizeof(filename), 0);

        if (received <= 0) {
            cerr << "Mất kết nối với server.\n";
            return; // Thoát nếu mất kết nối
        }

        string fname(filename);

        // Kiểm tra nếu nhận được "EOF"
        if (fname == "EOF") {
            break; // Dừng vòng lặp khi nhận được EOF
        }

        cout << fname << endl; // In tên file/thư mục
    }

    // Khi nhận xong danh sách file, quay lại menu
}

// Hàm tải xuống file từ server
void download_file(int sock) {
    cout << "Nhập tên file cần tải xuống: ";
    string filename;
    getline(cin, filename);

    if (filename.empty()) {
        cerr << "Tên file không được để trống.\n";
        return;
    }

    // Gửi lựa chọn '2'
    char choice = '2';
    if (!send_all(sock, &choice, sizeof(choice))) {
        cerr << "Gửi lựa chọn thất bại.\n";
        return;
    }

    // Gửi tên file (bao gồm null terminator)
    if (!send_all(sock, filename.c_str(), filename.size() + 1)) {
        cerr << "Gửi tên file thất bại.\n";
        return;
    }

    // Nhận nội dung file
    ofstream outfile(filename, ios::binary);
    if (!outfile.is_open()) {
        cerr << "Không thể tạo file để lưu.\n";
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t received;
    while ((received = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        // Kiểm tra xem đã đến EOF chưa
        string temp(buffer, received);      // chuyển data nhận được vào biến temp 
        if (temp.find("EOF") != string::npos) {
            // Xử lý nếu nhận được EOF
            break;
        }
        outfile.write(buffer, received);    // ghi tiếp phần data nhận được vào cuối file
    }

    if (received == -1) {
        perror("recv");
    } else {
        cout << "Tải xuống file '" << filename << "' thành công.\n";
    }

    outfile.close();
}

// Hàm tải lên file lên server
void upload_file(int sock) {
    cout << "Nhập tên file cần tải lên: ";
    string filename;
    getline(cin, filename);

    if (filename.empty()) {
        cerr << "Tên file không được để trống.\n";
        return;
    }

    // Mở file để đọc
    ifstream infile(filename, ios::binary);
    if (!infile.is_open()) {
        cerr << "Không thể mở file '" << filename << "' để đọc.\n";
        return;
    }

    // Gửi lựa chọn '3'
    char choice = '3';
    if (!send_all(sock, &choice, sizeof(choice))) {
        cerr << "Gửi lựa chọn thất bại.\n";
        return;
    }

    // Gửi tên file (bao gồm null terminator)
    if (!send_all(sock, filename.c_str(), filename.size() + 1)) {
        cerr << "Gửi tên file thất bại.\n";
        return;
    }

    // Gửi nội dung file
    char buffer[BUFFER_SIZE];
    while (infile.read(buffer, sizeof(buffer))) {
        if (!send_all(sock, buffer, infile.gcount())) {
            cerr << "Gửi dữ liệu file thất bại.\n";
            infile.close();
            return;
        }
    }
    // Gửi phần còn lại
    if (infile.gcount() > 0) {
        if (!send_all(sock, buffer, infile.gcount())) {
            cerr << "Gửi dữ liệu file thất bại.\n";
            infile.close();
            return;
        }
    }

    infile.close();
    cout << "Tải lên file '" << filename << "' thành công.\n";
}

// Hàm ngắt kết nối
void exit_client(int sock) {
    // Gửi lựa chọn '4' để ngắt kết nối
    char choice = '4';
    send_all(sock, &choice, sizeof(choice));
    cout << "Đã ngắt kết nối với server.\n";
}

// Hàm khôi phục terminal
void reset_terminal_mode() {
    struct termios t;
    tcgetattr(STDIN_FILENO, &t); // Lấy thông tin cài đặt terminal hiện tại
    t.c_lflag |= ICANON | ECHO; // Bật lại chế độ canonical và echo
    tcsetattr(STDIN_FILENO, TCSANOW, &t); // Áp dụng thay đổi ngay lập tức
}