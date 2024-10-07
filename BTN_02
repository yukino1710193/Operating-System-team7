#!/bin/bash
export PATH=$PATH:$(pwd)

# Kiểm tra xem tham số đã được cung cấp chưa
if [ $# -eq 0 ]; then
    echo "Vui lòng cung cấp đường dẫn tới thư mục."
    exit 1
fi

# Gán tham số vào biến
DIRECTORY=$1

# Kiểm tra xem thư mục có tồn tại không
if [ ! -d "$DIRECTORY" ]; then
    echo "Thư mục '$DIRECTORY' không tồn tại."
    exit 1
fi

# Hàm để tìm và xử lý các tệp rỗng
process_empty_files() {
    local dir="$1"
    # Tìm các tệp có kích thước 0 trong thư mục hiện tại
    empty_files=$(find "$dir" -type f -size 0 | grep -v '^$')
    while read -r FILE; do
        # Bỏ qua nếu FILE là chuỗi rỗng
        if [ -z "$FILE" ]; then
            continue
        fi
    echo ''
    echo "Tệp rỗng: $FILE"
    ./menu "Có" "Không" </dev/tty
    case $? in
        1)
            rm "$FILE"
            echo "Đã xóa: $FILE"
            ;;
        2)
            echo "Giữ lại: $FILE"
            ;;
    esac
    done <<< "$empty_files"
    
sub_dirs=$(find "$dir" -mindepth 1 -type d)
for sub_dir in $sub_dirs; do
        process_empty_files "$sub_dir"  # Gọi lại hàm cho thư mục con
    done
}

# Gọi hàm để xử lý các tệp rỗng
process_empty_files "$DIRECTORY"
