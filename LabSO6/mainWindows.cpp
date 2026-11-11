#include <iostream>
#include <string>
#include <windows.h>
#include <cstdlib>
#include <ctime>
#include <vector>

const std::string SHM_NAME = "Local\\ContorMemoriePartajata";
const std::string SEM_NAME = "Local\\SemaforContor";
const int MAX_COUNT = 1000;

class process
{
public:
    process(int processId, int* data, HANDLE semaphore):
        _processId{processId},
        _data{data},
        _semaphore{semaphore}
    {}

    virtual void run(void)
    {
        srand(time(NULL) ^ (GetCurrentProcessId() << 16)); 

        while (true) {
            WaitForSingleObject(_semaphore, INFINITE);

            if (*_data >= MAX_COUNT) {
                ReleaseSemaphore(_semaphore, 1, NULL);
                break;
            }

            int moneda = rand() % 2;
            while (moneda == 1 && *_data < MAX_COUNT) {
                (*_data)++;
                std::cout << _processId << " " << *_data << std::endl;
                
                moneda = rand() % 2;
            }

            ReleaseSemaphore(_semaphore, 1, NULL);
            Sleep(1); 
        }
    }

protected:
    int _processId;
    int* _data;
    HANDLE _semaphore;
};

class mainProcess : public process
{
public:
    mainProcess(int* data, HANDLE semaphore) 
        : process(1, data, semaphore) {}
};

class secondProcess : public process
{
public:
    secondProcess(int* data, HANDLE semaphore) 
        : process(2, data, semaphore) {}
};

int main(int argc, char* argv[]) {
    HANDLE hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(int), SHM_NAME.c_str());

    int* contor_partajat = (int*)MapViewOfFile(
        hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(int));

    HANDLE semafor = CreateSemaphoreA(
        NULL, 1, 1, SEM_NAME.c_str());

    if (argc == 1) { 
        *contor_partajat = 0;

        STARTUPINFOA si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        std::string cmd = std::string(argv[0]) + " child";
        std::vector<char> cmdBuffer(cmd.begin(), cmd.end());
        cmdBuffer.push_back('\0');

        CreateProcessA(NULL, cmdBuffer.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

        mainProcess p1(contor_partajat, semafor);
        p1.run();
        
        WaitForSingleObject(pi.hProcess, INFINITE);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

    } else {
        secondProcess p2(contor_partajat, semafor);
        p2.run();
    }

    UnmapViewOfFile(contor_partajat);
    CloseHandle(hMapFile);
    CloseHandle(semafor);

    return 0;
}