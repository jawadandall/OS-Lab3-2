#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>

#define SHM_KEY 0x1234  // Unique shared memory key
#define NUM_LOOPS 25    // Number of loops for each process

// Shared variables in memory structure
struct shared_data {
    int BankAccount;
    int Turn;
};

// Dad function to deposit money
void dad(struct shared_data *data) {
    int account;

    srand(time(NULL) ^ getpid());  // Seed the random generator uniquely for each process
    int sleep_time = rand() % 6;
    sleep(sleep_time);

    while (data->Turn != 0);  // Strict alternation: wait for Turn == 0
    account = data->BankAccount;
    
    if (account <= 100) {
        int deposit = rand() % 101;
        if (deposit % 2 == 0) {
            account += deposit;
            data->BankAccount = account;
            printf("Dear old Dad: Deposits $%d / Balance = $%d\n", deposit, account);
        } else {
            printf("Dear old Dad: Doesn't have any money to give\n");
        }
    } else {
        printf("Dear old Dad: Thinks Student has enough Cash ($%d)\n", account);
    }

    data->Turn = 1;  // Set Turn to 1 for Student
}

// Student function to withdraw money
void student(struct shared_data *data) {
    int account;

    srand(time(NULL) ^ getpid());  // Seed random generator uniquely for each process
    int sleep_time = rand() % 6;
    sleep(sleep_time);

    while (data->Turn != 1);  // Strict alternation: wait for Turn == 1

    int needed = rand() % 51;
    printf("Poor Student needs $%d\n", needed);
    account = data->BankAccount;
    if (needed <= account) {
        account -= needed;
        data->BankAccount = account;
        printf("Poor Student: Withdraws $%d / Balance = $%d\n", needed, account);
    } else {
        printf("Poor Student: Not Enough Cash ($%d)\n", account);
    }


    data->Turn = 0;  // Set Turn to 0 for Dad
}

int main() {
    int shmid;
    struct shared_data *data;

    // Allocate shared memory
    shmid = shmget(SHM_KEY, sizeof(struct shared_data), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }

    // Attach the shared memory
    data = (struct shared_data *)shmat(shmid, NULL, 0);
    if (data == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

    // Initialize shared data
    data->BankAccount = 0;  // Initial bank balance
    data->Turn = 0;           // Start with Dad's turn

    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    } else if (pid > 0) {
        // Parent process (Dad)
        for (int i = 0; i < NUM_LOOPS; i++) {
            dad(data);
        }

        // Wait for the child process to complete
        wait(NULL);

        // Detach and remove shared memory
        shmdt(data);
        shmctl(shmid, IPC_RMID, NULL);
        printf("Parent process complete.\n");

    } else {
        // Child process (Student)
        for (int i = 0; i < NUM_LOOPS; i++) {
            student(data);
        }

        // Detach shared memory
        shmdt(data);
        printf("Child process complete.\n");
    }

    return 0;
}
