#include <windows.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>


enum class AccessLevel { PUBLIC, PRIVATE };


struct File {
    std::wstring name;
    std::wstring content;
    std::wstring owner;
    AccessLevel access;
    bool is_locked_for_write;
    std::wstring locked_by;

    size_t get_size() const { return content.size() * sizeof(wchar_t); }
};


class FileSystemManager {
private:
    std::unordered_map<std::wstring, std::shared_ptr<File>> storage;
    std::wstring current_user;
    std::wstring logs;

public:
    FileSystemManager(const std::wstring& default_user) : current_user(default_user) {
        add_log(L"Систему запущено. Користувач: " + current_user);
    }

    void add_log(const std::wstring& message) {
        logs += message + L"\r\n";
    }

    std::wstring get_logs() const { return logs; }
    std::wstring get_current_user() const { return current_user; }
    const std::unordered_map<std::wstring, std::shared_ptr<File>>& get_storage() const { return storage; }

    void switch_user(const std::wstring& username) {
        current_user = username;
        add_log(L"Користувача змінено на: " + current_user);
    }

    void create_file(const std::wstring& name, AccessLevel access) {
        if (name.empty()) {
            add_log(L"Помилка: Введіть ім'я файлу!");
            return;
        }
        if (storage.find(name) != storage.end()) {
            add_log(L"Помилка: Файл \"" + name + L"\" вже існує.");
            return;
        }

        auto new_file = std::make_shared<File>();
        new_file->name = name;
        new_file->content = L"";
        new_file->owner = current_user;
        new_file->access = access;
        new_file->is_locked_for_write = false;
        new_file->locked_by = L"";

        storage[name] = new_file;
        std::wstring type = (access == AccessLevel::PUBLIC) ? L"Public" : L"Private";
        add_log(L" Створено " + type + L" файл \"" + name + L"\" (Власник: " + current_user + L")");
    }
    void open_file_for_edit(const std::wstring& name) {
        if (storage.find(name) == storage.end()) {
            add_log(L"Помилка: Файлу \"" + name + L"\" не існує.");
            return;
        }

        auto file = storage[name];
        if (file->access == AccessLevel::PRIVATE && file->owner != current_user) {
            add_log(L"Доступ заборонено! Файл приватний і належить " + file->owner);
            return;
        }

        if (file->is_locked_for_write) {
            add_log(L"Файл вже редагує " + file->locked_by + L". [READ-ONLY]");
            add_log(L"Вміст: " + (file->content.empty() ? L"[Порожньо]" : file->content));
            return;
        }

        file->is_locked_for_write = true;
        file->locked_by = current_user;
        add_log(L" Файл \"" + name + L"\" відкрито користувачем " + current_user);
    }

    void write_file(const std::wstring& name, const std::wstring& new_content) {
        if (storage.find(name) == storage.end()) return;
        auto file = storage[name];

        if (!file->is_locked_for_write || file->locked_by != current_user) {
            add_log(L"Помилка запису: Файл не відкрито вами.");
            return;
        }

        file->content = new_content;
        add_log(L"Записано дані у \"" + name + L"\"");
    }

    void close_file(const std::wstring& name) {
        if (storage.find(name) == storage.end()) return;
        auto file = storage[name];

        if (file->is_locked_for_write && file->locked_by == current_user) {
            file->is_locked_for_write = false;
            file->locked_by = L"";
            add_log(L"Файл " + name + L" збережено та закрито.");
        }
    }

    void rename_file(const std::wstring& old_name, const std::wstring& new_name) {
        if (old_name.empty() || new_name.empty()) {
            add_log(L"Помилка: Заповніть обидва поля імен!");
            return;
        }
        if (storage.find(old_name) == storage.end()) {
            add_log(L"Помилка: Файлу \"" + old_name + L"\" не існує.");
            return;
        }
        if (storage.find(new_name) != storage.end()) {
            add_log(L"Помилка: Нове ім'я \"" + new_name + L"\" вже зайняте.");
            return;
        }

        auto file = storage[old_name];
        if (file->access == AccessLevel::PRIVATE && file->owner != current_user) {
            add_log(L"Тільки власник може перейменувати цей файл.");
            return;
        }
        if (file->is_locked_for_write) {
            add_log(L"Файл зараз відкритий або заблокований.");
            return;
        }

        file->name = new_name;
        storage[new_name] = file;
        storage.erase(old_name);
        add_log(L"Файл \"" + old_name + L"\" перейменовано на \"" + new_name + L"\"");
    }

    void delete_file(const std::wstring& name) {
        if (name.empty()) {
            add_log(L"Помилка: Введіть ім'я файлу для видалення!");
            return;
        }
        if (storage.find(name) == storage.end()) {
            add_log(L"Помилка: Файлу \"" + name + L"\" не існує.");
            return;
        }

        auto file = storage[name];
        if (file->access == AccessLevel::PRIVATE && file->owner != current_user) {
            add_log(L"Тільки власник може видалити цей файл.");
            return;
        }
        if (file->is_locked_for_write) {
            add_log(L"Файл зараз відкритий користувачем " + file->locked_by);
            return;
        }

        storage.erase(name);
        add_log(L"Файл \"" + name + L"\" повністю видалено.");
    }
};

FileSystemManager fs(L"Admin");
HWND hLogBox, hDirBox, hFileInput, hNewNameInput, hContentInput, hUserText;

#define BTN_USER_ADMIN 1
#define BTN_USER_USER1 2
#define BTN_CREATE_PUB 3
#define BTN_CREATE_PRV 4
#define BTN_OPEN_FILE  5
#define BTN_WRITE_FILE 6
#define BTN_CLOSE_FILE 7
#define BTN_RENAME_FILE 8
#define BTN_DELETE_FILE 9

void UpdateUI() {
    SetWindowTextW(hLogBox, fs.get_logs().c_str());
    std::wstring user_status = L"Поточний користувач: " + fs.get_current_user();
    SetWindowTextW(hUserText, user_status.c_str());

    std::wstring dir_content = L"";
    const auto& files = fs.get_storage();
    if (files.empty()) {
        dir_content = L"[Директорія порожня. Створіть перший файл]";
    } else {
        for (const auto& [name, file] : files) {
            std::wstring access_str = (file->access == AccessLevel::PUBLIC) ? L"Public" : L"Private";
            std::wstring lock_str = file->is_locked_for_write ? (L"Зайнятий (" + file->locked_by + L")") : L"Вільний";
            dir_content += L" " + file->name + L" | Розмір: " + std::to_wstring(file->get_size()) + L" б | Доступ: " + access_str + L" | Стан: " + lock_str + L"\r\n";
        }
    }
    SetWindowTextW(hDirBox, dir_content.c_str());
}
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    wchar_t fileNameBuf[256] = { 0 };
    wchar_t newNameBuf[256] = { 0 };
    wchar_t contentBuf[1024] = { 0 };

    switch (uMsg) {
    case WM_CREATE:
        hUserText = CreateWindowW(L"STATIC", L"Поточний користувач: Admin", WS_VISIBLE | WS_CHILD, 20, 15, 300, 20, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Увійти як Admin", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 40, 140, 30, hwnd, (HMENU)BTN_USER_ADMIN, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Увійти як User1", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 40, 140, 30, hwnd, (HMENU)BTN_USER_USER1, NULL, NULL);

        CreateWindowW(L"STATIC", L"Ім'я файлу / Старе ім'я:", WS_VISIBLE | WS_CHILD, 20, 90, 200, 20, hwnd, NULL, NULL, NULL);
        hFileInput = CreateWindowW(L"EDIT", L"test.txt", WS_VISIBLE | WS_CHILD | WS_BORDER, 20, 110, 140, 20, hwnd, NULL, NULL, NULL);

        CreateWindowW(L"STATIC", L"Нове ім'я (для перейм.):", WS_VISIBLE | WS_CHILD, 170, 90, 200, 20, hwnd, NULL, NULL, NULL);
        hNewNameInput = CreateWindowW(L"EDIT", L"new_test.txt", WS_VISIBLE | WS_CHILD | WS_BORDER, 170, 110, 140, 20, hwnd, NULL, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Створити Public", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 140, 140, 30, hwnd, (HMENU)BTN_CREATE_PUB, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Створити Private", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 140, 140, 30, hwnd, (HMENU)BTN_CREATE_PRV, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Перейменувати", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 180, 140, 30, hwnd, (HMENU)BTN_RENAME_FILE, NULL, NULL);
        CreateWindowW(L"BUTTON", L" Видалити файл", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 170, 180, 140, 30, hwnd, (HMENU)BTN_DELETE_FILE, NULL, NULL);

        CreateWindowW(L"STATIC", L"Текст для запису у файл:", WS_VISIBLE | WS_CHILD, 20, 230, 200, 20, hwnd, NULL, NULL, NULL);
        hContentInput = CreateWindowW(L"EDIT", L"Привіт Світ!", WS_VISIBLE | WS_CHILD | WS_BORDER, 20, 250, 290, 20, hwnd, NULL, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Відкрити", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 20, 280, 90, 30, hwnd, (HMENU)BTN_OPEN_FILE, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Записати", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 115, 280, 100, 30, hwnd, (HMENU)BTN_WRITE_FILE, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Закрити", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 220, 280, 90, 30, hwnd, (HMENU)BTN_CLOSE_FILE, NULL, NULL);

        CreateWindowW(L"STATIC", L" Поточна директорія сховища (Список файлів):", WS_VISIBLE | WS_CHILD, 340, 15, 400, 20, hwnd, NULL, NULL, NULL);
        hDirBox = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY, 340, 35, 430, 130, hwnd, NULL, NULL, NULL);

        CreateWindowW(L"STATIC", L" Журнал подій файлової системи:", WS_VISIBLE | WS_CHILD, 340, 180, 400, 20, hwnd, NULL, NULL, NULL);
        hLogBox = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY, 340, 200, 430, 110, hwnd, NULL, NULL, NULL);
        
        UpdateUI();
        break;

    case WM_COMMAND:
        GetWindowTextW(hFileInput, fileNameBuf, 256);
        GetWindowTextW(hNewNameInput, newNameBuf, 256);
        GetWindowTextW(hContentInput, contentBuf, 1024);

        switch (LOWORD(wParam)) {
        case BTN_USER_ADMIN: fs.switch_user(L"Admin"); break;
        case BTN_USER_USER1: fs.switch_user(L"User1"); break;
        case BTN_CREATE_PUB: fs.create_file(fileNameBuf, AccessLevel::PUBLIC); break;
        case BTN_CREATE_PRV: fs.create_file(fileNameBuf, AccessLevel::PRIVATE); break;
        case BTN_OPEN_FILE:  fs.open_file_for_edit(fileNameBuf); break;
        case BTN_WRITE_FILE: fs.write_file(fileNameBuf, contentBuf); break;
        case BTN_CLOSE_FILE: fs.close_file(fileNameBuf); break;
        case BTN_RENAME_FILE: fs.rename_file(fileNameBuf, newNameBuf); break;
        case BTN_DELETE_FILE: fs.delete_file(fileNameBuf); break;
        }
        UpdateUI();
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"FSWindowClassNameFinalSplit";

    WNDCLASSW wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(0, CLASS_NAME, L"Файловий Менеджер", 
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, 
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 370, NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
