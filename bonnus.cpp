#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>

const int MAX_CONTACTS = 100;
const int NAME_MAX_LENGTH = 100;
const char *FILE_NAME = "contacts.bin";

struct Contact {
    int yob;
    char name[NAME_MAX_LENGTH];
    char telnum[NAME_MAX_LENGTH];
};

struct Header {
    char names[MAX_CONTACTS][NAME_MAX_LENGTH];
    int count;
};

void initializeFile() {
    std::fstream file(FILE_NAME, std::ios::binary | std::ios::out | std::ios::in);
    if (!file) {
        Header header = {};
        header.count = 0;
        std::ofstream outFile(FILE_NAME, std::ios::binary);
        outFile.write(reinterpret_cast<const char *>(&header), sizeof(Header));
        outFile.close();
    }
}

void addContact() {
    std::fstream file(FILE_NAME, std::ios::binary | std::ios::in | std::ios::out);

    Header header;
    file.read(reinterpret_cast<char *>(&header), sizeof(Header));

    if (header.count >= MAX_CONTACTS) {
        std::cout << "Danh bạ quá giới hạn." << std::endl;
        return;
    }

    Contact contact;
    std::cout << "Nhập tên: ";
    std::cin.ignore();
    std::cin.getline(contact.name, NAME_MAX_LENGTH);
    std::cout << "Nhập năm sinh: ";
    std::cin >> contact.yob;
    std::cin.ignore();
    std::cout << "Nhập số điện thoại: ";
    std::cin.getline(contact.telnum, NAME_MAX_LENGTH);

    // Add name to header
    std::strncpy(header.names[header.count], contact.name, NAME_MAX_LENGTH);
    header.count++;

    // Write updated header
    file.seekp(0, std::ios::beg);
    file.write(reinterpret_cast<const char *>(&header), sizeof(Header));

    // Append contact to file
    file.seekp(0, std::ios::end);
    file.write(reinterpret_cast<const char *>(&contact), sizeof(Contact));

    std::cout << "Thêm contact thành công!" << std::endl;
    file.close();
}

void findContact() {
    std::ifstream file(FILE_NAME, std::ios::binary);

    Header header;
    file.read(reinterpret_cast<char *>(&header), sizeof(Header));

    char searchName[NAME_MAX_LENGTH];
    std::cout << "Nhập tên cần tìm: ";
    std::cin.ignore();
    std::cin.getline(searchName, NAME_MAX_LENGTH);

    for (int i = 0; i < header.count; i++) {
        if (std::strcmp(header.names[i], searchName) == 0) {
            // Found, now read the contact
            file.seekg(sizeof(Header) + i * sizeof(Contact), std::ios::beg);
            Contact contact;
            file.read(reinterpret_cast<char *>(&contact), sizeof(Contact));

            std::cout << "Số điện thoại: " << contact.telnum << std::endl;
            file.close();
            return;
        }
    }

    std::cout << "Không tìm thấy contact!" << std::endl;
    file.close();
}

void deleteContact() {
    std::fstream file(FILE_NAME, std::ios::binary | std::ios::in | std::ios::out);

    Header header;
    file.read(reinterpret_cast<char *>(&header), sizeof(Header));

    char deleteName[NAME_MAX_LENGTH];
    std::cout << "Nhập tên cần xóa: ";
    std::cin.ignore();
    std::cin.getline(deleteName, NAME_MAX_LENGTH);

    for (int i = 0; i < header.count; i++) {
        if (std::strcmp(header.names[i], deleteName) == 0) {
            // Remove the contact
            for (int j = i; j < header.count - 1; j++) {
                std::strncpy(header.names[j], header.names[j + 1], NAME_MAX_LENGTH);
            }
            header.count--;

            // Write updated header
            file.seekp(0, std::ios::beg);
            file.write(reinterpret_cast<const char *>(&header), sizeof(Header));

            // Rewrite contacts after the deleted one
            std::vector<Contact> contacts;
            Contact temp;
            file.seekg(sizeof(Header), std::ios::beg);
            for (int j = 0; j < header.count; j++) {
                file.read(reinterpret_cast<char *>(&temp), sizeof(Contact));
                contacts.push_back(temp);
            }

            file.close();
            std::ofstream outFile(FILE_NAME, std::ios::binary);
            outFile.write(reinterpret_cast<const char *>(&header), sizeof(Header));
            for (const auto &c : contacts) {
                outFile.write(reinterpret_cast<const char *>(&c), sizeof(Contact));
            }

            std::cout << "Xóa contact thành công!" << std::endl;
            outFile.close();
            return;
        }
    }

    std::cout << "Không tìm thấy contact!" << std::endl;
    file.close();
}

int main() {
    initializeFile();

    int choice;
    do {
        std::cout << "\nQUẢN LÝ DANH BẠ:\n";
        std::cout << "1. Thêm contact\n";
        std::cout << "2. Tìm contact\n";
        std::cout << "3. Xóa contact\n";
        std::cout << "4. Thoát\n";
        std::cout << "Nhập lựa chọn: ";
        std::cin >> choice;

        switch (choice) {
            case 1:
                addContact();
                break;
            case 2:
                findContact();
                break;
            case 3:
                deleteContact();
                break;
            case 4:
                std::cout << "Thoát chương trình." << std::endl;
                break;
            default:
                std::cout << "Lựa chọn không hợp lệ." << std::endl;
        }
    } while (choice != 4);

    return 0;
}
