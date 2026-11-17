#include <iostream>
#include <fstream>
#include <windows.h>
#include <string>
#include <cstring>

using namespace std;

struct Header {
    int head;
    int tail;
    int capacity;
};

struct Message {
    char text[20];
};

int main(int argc, char* argv[]) {
    system("chcp 1251 > nul");
    setlocale(LC_ALL, "Russian");

    if (argc < 3) {
        cerr << "Недостаточно аргументов командной строки!" << endl;
        return 1;
    }

    string filename = argv[1];
    string sender_id = argv[2];

    fstream test_file(filename, ios::binary | ios::in | ios::out);
    if (!test_file.is_open()) {
        cerr << "Ошибка открытия файла!" << endl;
        return 1;
    }
    test_file.close();

    HANDLE hMutex = OpenMutexA(SYNCHRONIZE, FALSE, "Lab4_FileMutex");
    HANDLE hEmptySemaphore = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, "Lab4_EmptySem");
    HANDLE hFilledSemaphore = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, "Lab4_FilledSem");

    string event_name = "Lab4_SenderReady_" + sender_id;
    HANDLE hReadyEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, event_name.c_str());

    SetEvent(hReadyEvent);
    cout << "[Sender " << sender_id << "] Готов к работе. Файл: " << filename << endl;

    int choice = 0;
    while (true) {
        cout << "\nВыберите действие (1 - отправить сообщение, 0 - завершить работу): ";
        cin >> choice;

        if (choice == 0) {
            break;
        }
        else if (choice == 1) {
            string input_text;
            cout << "Введите текст сообщения (меньше 20 символов): ";
            cin.ignore();
            getline(cin, input_text);

            if (input_text.length() >= 20) {
                cout << "Ошибка: длина сообщения должна быть меньше 20 символов!" << endl;
                continue;
            }

            cout << "[Sender " << sender_id << "] Ожидание свободного места в файле..." << endl;
            WaitForSingleObject(hEmptySemaphore, INFINITE);
            WaitForSingleObject(hMutex, INFINITE);

            Header header;
            fstream io_file(filename, ios::binary | ios::in | ios::out);
            io_file.read(reinterpret_cast<char*>(&header), sizeof(Header));

            Message msg = {};
            strcpy_s(msg.text, input_text.c_str());

            io_file.seekp(sizeof(Header) + header.tail * sizeof(Message), ios::beg);
            io_file.write(reinterpret_cast<char*>(&msg), sizeof(Message));

            header.tail = (header.tail + 1) % header.capacity;
            io_file.seekp(0, ios::beg);
            io_file.write(reinterpret_cast<char*>(&header), sizeof(Header));
            io_file.close();

            cout << "[Sender " << sender_id << "] Сообщение отправлено!" << endl;

            ReleaseMutex(hMutex);
            ReleaseSemaphore(hFilledSemaphore, 1, NULL);
        }
    }

    CloseHandle(hReadyEvent);
    CloseHandle(hMutex);
    CloseHandle(hEmptySemaphore);
    CloseHandle(hFilledSemaphore);

    cout << "[Sender " << sender_id << "] Работа завершена." << endl;
    return 0;
}