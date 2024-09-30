#include <stdio.h>

int display_menu(char *options[], int n) {
    int choice = -1;
    
    // In menu
    printf("Menu:\n");
    for (int i = 0; i < n; i++) {
        printf("%d. %s\n", i + 1, options[i]);
    }
    
    // Nhận lựa chọn từ người dùng
    do {
        printf("Hãy chọn một tùy chọn (1-%d): ", n);
        scanf("%d", &choice);
        
        // Kiểm tra xem lựa chọn có hợp lệ không
        if (choice < 1 || choice > n) {
            printf("Lựa chọn không hợp lệ, vui lòng thử lại.\n");
        }
    } while (choice < 1 || choice > n);
    
    return choice;
}

int main(int argc, char *argv[]) {
    // Kiểm tra nếu không có lựa chọn nào được truyền vào
    if (argc < 2) {
        printf("Vui lòng cung cấp các tùy chọn menu qua dòng lệnh.\n");
        return 1;
    }

    // Hiển thị menu và nhận lựa chọn từ người dùng
    int selected_option = display_menu(&argv[1], argc - 1);
    
    // In lựa chọn của người dùng
    printf("Bạn đã chọn: %s\n", argv[selected_option]);

    return 0;
}