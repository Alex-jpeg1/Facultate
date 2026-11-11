#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <sys/wait.h>

const std::string SHM_NAME = "/contor_memorie_partajata";
const std::string SEM_NAME = "/semafor_contor";
const int MAX_COUNT = 1000;

class process
{
public:
    process(int processId, int* data, sem_t* semaphore):
        _processId{processId},
        _data{data},
        _semaphore{semaphore}
    {}

    virtual void run(void)
    {
        srand(time(NULL) ^ (getpid() << 16)); 

        while (true) {
            sem_wait(_semaphore);

            if (*_data >= MAX_COUNT) {
                sem_post(_semaphore);
                break;
            }

            int moneda = rand() % 2;
            while (moneda == 1 && *_data < MAX_COUNT) {
                (*_data)++;
                std::cout << _processId << " " << *_data << std::endl;
                
                moneda = rand() % 2;
            }

            sem_post(_semaphore);
            usleep(1000); 
        }
    }

protected:
    int _processId;
    int* _data;
    sem_t* _semaphore;
};

class mainProcess : public process
{
public:
    mainProcess(int* data, sem_t* semaphore) 
        : process(1, data, semaphore) {}
};

class secondProcess : public process
{
public:
    secondProcess(int* data, sem_t* semaphore) 
        : process(2, data, semaphore) {}
};

int main() {
    int shm_fd = shm_open(SHM_NAME.c_str(), O_CREAT | O_RDWR, 0666);

    ftruncate(shm_fd, sizeof(int));

    int* contor_partajat = (int*)mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    *contor_partajat = 0;

    sem_t* semafor = sem_open(SEM_NAME.c_str(), O_CREAT, 0666, 1);
    pid_t pid = fork();
    if (pid == 0) {
        secondProcess p2(contor_partajat, semafor);
        p2.run();
            
        exit(0);
    } 
    else {
        mainProcess p1(contor_partajat, semafor);
        p1.run();
        
        wait(NULL);

        munmap(contor_partajat, sizeof(int));
        shm_unlink(SHM_NAME.c_str());
        sem_close(semafor);
        sem_unlink(SEM_NAME.c_str());
    }

    return 0;
}