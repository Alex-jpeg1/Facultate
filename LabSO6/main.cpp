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

enum class HowManyTimes
{
    ONCE,
    MORE_THAN_ONCE
};

class Semaphore
{
public:
    Semaphore(sem_t* semaphore) :
        open{true},
        _semaphore{semaphore}
    {}

    void Wait()
    {
        sem_wait(_semaphore);
        open = false;
    }

    void Open(HowManyTimes openAmount)
    {
        if(openAmount == HowManyTimes::ONCE)
        {
            if(open)
            {
                return;
            }
            else 
            {
                open = true;
                sem_post(_semaphore);
            }
        }
        else
        {
            open = true;
            sem_post(_semaphore);
        }
    }

private:
    bool open;
    sem_t* _semaphore;
};

class process
{
public:
    process(int processId, int* data, Semaphore* semaphore):
        _processId{processId},
        _data{data},
        _semaphore{semaphore}
    {}

    virtual void run(void)
    {
        srand(time(NULL) ^ (getpid() << 16)); 

        while (true) {
            _semaphore->Wait();

            if (*_data >= MAX_COUNT) {
                _semaphore->Open(HowManyTimes::ONCE);
                break;
            }

            int moneda = rand() % 2;
            while (moneda == 1 && *_data < MAX_COUNT) {
                (*_data)++;
                std::cout << _processId << " " << *_data << std::endl;
                
                moneda = rand() % 2;
            }

            _semaphore->Open(HowManyTimes::ONCE);
            usleep(1000); 
        }
    }

protected:
    int _processId;
    int* _data;
    Semaphore* _semaphore;
};

class mainProcess : public process
{
public:
    mainProcess(int* data, Semaphore* semaphore) 
        : process(1, data, semaphore) {}
};

class secondProcess : public process
{
public:
    secondProcess(int* data, Semaphore* semaphore) 
        : process(2, data, semaphore) {}
};

int main() {
    int shm_fd = shm_open(SHM_NAME.c_str(), O_CREAT | O_RDWR, 0666);

    ftruncate(shm_fd, sizeof(int));

    int* contor_partajat = (int*)mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    *contor_partajat = 0;

    sem_t* semafor = sem_open(SEM_NAME.c_str(), O_CREAT, 0666, 1);
    
    Semaphore semaforWrapper(semafor);

    pid_t pid = fork();
    if (pid == 0) {
        secondProcess p2(contor_partajat, &semaforWrapper);
        p2.run();

        exit(0);
    } 
    else {
        mainProcess p1(contor_partajat, &semaforWrapper);
        p1.run();
        
        wait(NULL);

        munmap(contor_partajat, sizeof(int));
        shm_unlink(SHM_NAME.c_str());
        sem_close(semafor);
        sem_unlink(SEM_NAME.c_str());
    }

    return 0;
}