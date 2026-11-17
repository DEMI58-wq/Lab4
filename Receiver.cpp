#include <iostream>
#include <fstream>
#include <windows.h>
#include <vector>
#include <string>

using namespace std;

struct Header {
    int head;
    int tail;
    int capacity;
};

struct Message {
    char text[20];
};

int main() {
    system("chcp 1251 > nul");
    setlocale(LC_ALL, "Russian");

    string filename;
    int record_count = 0;

    cout << "Введите имя бинарного файла: ";
    cin >> filename;

    cout << "Введите количество записей в файле: ";
    cin >> record_count;

    if (record_count <= 0) {
        cerr << "Количество записей должно быть больше нуля!" << endl;
        return 1;
    }

    fstream file(filename, ios::binary | ios::out | ios::trunc);
    if (!file.is_open()) {
        cerr << "Ошибка создания файла!" << endl;
        return 1;
    }

    Header header = { 0, 0, record_count };
    file.write(reinterpret_cast<char*>(&header), sizeof(Header));

    Message empty_msg = {};
    for (int i = 0; i < record_count; i++) {
        file.write(reinterpret_cast<char*>(&empty_msg), sizeof(Message));
    }
    file.close();

    HANDLE hMutex = CreateMutexA(NULL, FALSE, "Lab4_FileMutex");
    HANDLE hEmptySemaphore = CreateSemaphoreA(NULL, record_count, record_count, "Lab4_EmptySem");
    HANDLE hFilledSemaphore = CreateSemaphoreA(NULL, 0, record_count, "Lab4_FilledSem");

    int sender_count = 0;
    cout << "Введите количество процессов Sender: ";
    cin >> sender_count;

    if (sender_count <= 0) {
        cerr << "Количество процессов должно быть больше нуля!" << endl;
        CloseHandle(hMutex);
        CloseHandle(hEmptySemaphore);
        CloseHandle(hFilledSemaphore);
        return 1;
    }

    vector<HANDLE> hReadyEvents(sender_count);
    vector<PROCESS_INFORMATION> pi(sender_count);

    for (int i = 0; i < sender_count; i++) {
        string event_name = "Lab4_SenderReady_" + to_string(i);
        hReadyEvents[i] = CreateEventA(NULL, TRUE, FALSE, event_name.c_str());

        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi[i], sizeof(pi[i]));

        string cmd = "Sender.exe " + filename + " " + to_string(i);
        vector<char> cmd_buf(cmd.begin(), cmd.end());
        cmd_buf.push_back('\0');

        if (!CreateProcessA(NULL, cmd_buf.data(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi[i])) {
            cerr << "Ошибка запуска процесса Sender " << i + 1 << endl;
        }
    }

    cout << "\n[Receiver] Ожидание готовности всех процессов Sender..." << endl;
    WaitForMultipleObjects(sender_count, hReadyEvents.data(), TRUE, INFINITE);
    cout << "[Receiver] Все процессы Sender готовы к работе!\n" << endl;

    int choice = 0;
    while (true) {
        cout << "\nВыберите действие (1 - читать сообщение, 0 - завершить работу): ";
        cin >> choice;

        if (choice == 0) {
            break;
        }
        else if (choice == 1) {
            cout << "[Receiver] Ожидание сообщения..." << endl;
            WaitForSingleObject(hFilledSemaphore, INFINITE);
            WaitForSingleObject(hMutex, INFINITE);

            fstream io_file(filename, ios::binary | ios::in | ios::out);
            io_file.read(reinterpret_cast<char*>(&header), sizeof(Header));

            Message msg = {};
            io_file.seekg(sizeof(Header) + header.head * sizeof(Message), ios::beg);
            io_file.read(reinterpret_cast<char*>(&msg), sizeof(Message));

            header.head = (header.head + 1) % header.capacity;
            io_file.seekp(0, ios::beg);
            io_file.write(reinterpret_cast<char*>(&header), sizeof(Header));
            io_file.close();

            cout << "[Receiver] Получено сообщение: " << msg.text << endl;

            ReleaseMutex(hMutex);
            ReleaseSemaphore(hEmptySemaphore, 1, NULL);
        }
    }

    for (int i = 0; i < sender_count; i++) {
        CloseHandle(hReadyEvents[i]);
        CloseHandle(pi[i].hThread);
        CloseHandle(pi[i].hProcess);
    }

    CloseHandle(hMutex);
    CloseHandle(hEmptySemaphore);
    CloseHandle(hFilledSemaphore);

    cout << "[Receiver] Работа завершена." << endl;
    return 0;
}