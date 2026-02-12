#include <iostream>
#include <filesystem>
#include <string>
#include <iomanip>

namespace fs = std::filesystem;

void list_directory(const fs::path& current_path) {
    std::cout << "\nПоточний шлях: " << current_path << "\n";
    std::cout << std::left << std::setw(10) << "Тип" << std::setw(30) << "Назва" << "Розмір" << "\n";
    std::cout << std::string(60, '-') << "\n";

    for (const auto& entry : fs::directory_iterator(current_path)) {
        std::string type = entry.is_directory() ? "[DIR]" : "[FILE]";
        std::string name = entry.path().filename().string();
        
        std::cout << std::left << std::setw(10) << type << std::setw(30) << name;
        
        if (entry.is_regular_file()) {
            std::cout << entry.file_size() << " bytes";
        }
        std::cout << "\n";
    }
}

int main() {
    fs::path current_p = fs::current_path();
    std::string command;

    while (true) {
        list_directory(current_p);
        std::cout << "\nКоманди: cd <назва>, mkdir <назва>, rm <назва>, exit\n> ";
        std::cin >> command;

        if (command == "exit") break;

        if (command == "cd") {
            std::string dirname;
            std::cin >> dirname;
            fs::path new_p = current_p / dirname; // Зручне склеювання шляхів

            if (fs::exists(new_p) && fs::is_directory(new_p)) {
                current_p = fs::canonical(new_p); // Перетворення у повний шлях
            } else {
                std::cout << "Помилка: Директорії не існує!\n";
            }
        } 
        else if (command == "mkdir") {
            std::string dirname;
            std::cin >> dirname;
            if (fs::create_directory(current_p / dirname)) {
                std::cout << "Папку створено.\n";
            }
        }
        else if (command == "rm") {
            std::string name;
            std::cin >> name;
            if (fs::remove_all(current_p / name)) { // Видаляє файл або папку з вмістом
                std::cout << "Видалено.\n";
            }
        }
    }
    return 0;
}