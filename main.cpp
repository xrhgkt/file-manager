#include <iostream>
#include <filesystem>
#include <string>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

class FileManager {
private:
    fs::path current_path;

    
    std::string format_time(fs::file_time_type ftime) {
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        std::time_t c_time = std::chrono::system_clock::to_time_t(sctp);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&c_time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

public:
    FileManager() : current_path(fs::current_path()) {}

    
    void list_contents() {
        try {
            std::cout << "\n[DIRECTORY]: " << current_path.string() << "\n";
            std::cout << std::left << std::setw(10) << "TYPE" 
                      << std::setw(25) << "NAME" 
                      << "LAST MODIFIED" << "\n";
            std::cout << std::string(60, '-') << "\n";

            for (const auto& entry : fs::directory_iterator(current_path)) {
                auto status = entry.status();
                std::cout << (fs::is_directory(status) ? "[DIR]     " : "[FILE]    ")
                          << std::left << std::setw(25) << entry.path().filename().string()
                          << format_time(fs::last_write_time(entry)) << "\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "Помилка доступу: " << e.what() << "\n";
        }
    }

  
    void change_directory(const std::string& path_str) {
        try {
            fs::path new_path = current_path / path_str;
            if (fs::exists(new_path) && fs::is_directory(new_path)) {
                current_path = fs::canonical(new_path);
            } else {
                std::cout << "Помилка: Директорію '" << path_str << "' не знайдено.\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "Некоректний шлях: " << e.what() << "\n";
        }
    }

   
    void make_directory(const std::string& name) {
        try {
            if (fs::create_directory(current_path / name)) {
                std::cout << "[OK] Директорію '" << name << "' створено.\n";
            } else {
                std::cout << "Помилка: Не вдалося створити директорію.\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "Помилка FS: " << e.what() << "\n";
        }
    }

    
    void create_file(const std::string& name) {
        try {
            fs::path file_p = current_path / name;
            std::ofstream file(file_p);
            if (file) {
                std::cout << "[OK] Файл '" << name << "' створено.\n";
            } else {
                std::cout << "Помилка: Не вдалося створити файл.\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "Помилка FS: " << e.what() << "\n";
        }
    }

  
    void rename_object(const std::string& old_n, const std::string& new_n) {
        try {
            fs::path old_p = current_path / old_n;
            fs::path new_p = current_path / new_n;
            if (fs::exists(old_p)) {
                fs::rename(old_p, new_p);
                std::cout << "[OK] '" << old_n << "' -> '" << new_n << "'\n";
            } else {
                std::cout << "Помилка: Об'єкт '" << old_n << "' не знайдено.\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "Помилка перейменування: " << e.what() << "\n";
        }
    }

  
    void remove_object(const std::string& name) {
        try {
            if (fs::remove_all(current_path / name)) {
                std::cout << "[OK] '" << name << "' видалено.\n";
            } else {
                std::cout << "Помилка: Об'єкт не знайдено.\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "Помилка видалення: " << e.what() << "\n";
        }
    }

    void run() {
        std::string line, cmd, arg1, arg2;
        std::cout << "--- C++17 File Manager Ready ---\n";
        std::cout << "Команди: ls, cd <dir>, mkdir <name>, touch <name>, rename <old> <new>, rm <name>, exit\n";

        while (true) {
            std::cout << "\n" << current_path.filename().string() << " > ";
            
            if (!std::getline(std::cin, line)) break;
            if (line.empty()) continue;

            std::stringstream ss(line);
            ss >> cmd;

            if (cmd == "ls") {
                list_contents();
            } else if (cmd == "cd") {
                if (ss >> arg1) change_directory(arg1);
            } else if (cmd == "mkdir") {
                if (ss >> arg1) make_directory(arg1);
            } else if (cmd == "touch") {
                if (ss >> arg1) create_file(arg1);
            } else if (cmd == "rename") {
                if (ss >> arg1 >> arg2) rename_object(arg1, arg2);
                else std::cout << "Використання: rename <стара_назва> <нова_назва>\n";
            } else if (cmd == "rm") {
                if (ss >> arg1) remove_object(arg1);
            } else if (cmd == "exit") {
                break;
            } else {
                std::cout << "Невідома команда: " << cmd << "\n";
            }
        }
    }
};

int main() {
    FileManager fm;
    fm.run();
    return 0;
}
