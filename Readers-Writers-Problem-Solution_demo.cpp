#include <iostream>
#include <mutex>
#include <semaphore>
#include <thread>
#include <vector>
#include <condition_variable>

std::mutex write_mutex;               // Mutex để bảo vệ phần ghi
std::mutex read_count_mutex;          // Mutex để bảo vệ biến đếm số lượng luồng đọc
std::counting_semaphore read_sem(10); // Semaphore để giới hạn số lượng luồng đọc đồng thời (tối đa 10)
std::condition_variable cv;           // Condition variable để thông báo khi ghi xong
int shared_resource = 0;              // Tài nguyên chia sẻ
int reader_count = 0;                 // Đếm số lượng luồng đọc đồng thời
bool is_writing = false;              // Cờ báo khi có ghi đang diễn ra

// Luồng ghi
void write(int id)
{
    std::cout << "Writer " << id << " is waiting to write..." << std::endl;

    // Bắt mutex để bảo vệ phần ghi
    std::lock_guard<std::mutex> lock(write_mutex); // Mutex cho ghi

    // Đánh dấu là đang ghi
    is_writing = true;

    // Thực hiện ghi
    shared_resource++;
    std::cout << "Writer " << id << " wrote, resource = " << shared_resource << std::endl;

    // Giải phóng mutex sau khi ghi xong
    is_writing = false;
    cv.notify_all(); // Thông báo cho tất cả các luồng đọc rằng ghi đã xong
}

// Luồng đọc
void read(int id)
{
    std::cout << "Reader " << id << " is waiting to read..." << std::endl;

    // Trước khi đọc, giảm semaphore (giới hạn số lượng luồng đọc đồng thời)
    read_sem.acquire();

    // Bảo vệ khi có ghi đang diễn ra, chặn các luồng đọc nếu có ghi
    std::unique_lock<std::mutex> lock(write_mutex);

    // Chờ nếu có ghi đang diễn ra
    cv.wait(lock, []()
            { return !is_writing; });

    // Đọc tài nguyên
    std::cout << "Reader " << id << " is reading, resource = " << shared_resource << std::endl;

    // Giải phóng mutex sau khi đọc xong
    lock.unlock();

    // Sau khi đọc xong, tăng semaphore để cho phép luồng khác đọc
    read_sem.release();
}

int main()
{
    std::vector<std::thread> threads;

    // Tạo 3 luồng ghi
    for (int i = 0; i < 3; ++i)
    {
        threads.push_back(std::thread([i]() { write(i); }));
    }

    // Tạo 12 luồng đọc
    for (int i = 0; i < 12; ++i)
    {
        threads.push_back(std::thread([i]() { read(i); })); // Tạo 12 luồng đọc
    }

    // Chờ tất cả các luồng hoàn thành
    for (auto &t : threads)
    {
        t.join();
    }

    return 0;
}
