#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/mman.h>
#define PTR_SIZE sizeof(int)
#define FIFO_NAME "fifo"
#define SRUN "sem_run"
#define SCLOSE "sem_close"
#define SUNLINK "sem_unlink"

struct thread_args {
    bool thread_run;
    pthread_t thread_id;
    thread_args() : thread_run(true) {};
    int exit_code;
    int filedes;
    sem_t* run_sem;
    sem_t* close_sem;
};

static void * thread_func(void *arg) {
    thread_args *args = (thread_args*) arg;
    printf("Started first thread\n");
    char message[4];    
   int pr_running = 1;
    int exit_code;
    // Synchronize processes
    printf("Synchronizing processes...\n");
    sem_post(args->run_sem);
    while (pr_running == 1) {
        sem_getvalue(args->run_sem, &pr_running);
    }
    printf("Processes synchronized\n");
    // Run thread
    while (args->thread_run) {
        // Sync exiting
        sem_getvalue(args->close_sem, &exit_code);
        if (!exit_code) {
            printf("Another process was finished, exiting...\n");
            sem_unlink(SCLOSE);
            break;
        }
        message[0] = '\0';
        read(args->filedes, message, 3);
        fflush(stdout);
        printf("Recieved: %s\n", message);
        sleep(1);
    }
    if (exit_code) sem_trywait(args->close_sem);
    args->exit_code = 1;
    printf("Finished first thread\n");
    pthread_exit((void*)&args->exit_code);
}

void unlink_old() {
    sem_t* s_unlink = sem_open(SUNLINK, O_CREAT, S_IRUSR | S_IWUSR, 1);
    int sem_value;
    sem_getvalue(s_unlink, &sem_value);
    printf("semvalue is %d\n", sem_value);
    if (sem_value) {
        sem_trywait(s_unlink);
        unlink(FIFO_NAME);
        sem_unlink(SRUN);
        sem_unlink(SCLOSE);
    }
}

int main() {
    thread_args arg;
    // Unlinking all garbage
    unlink_old();
    mkfifo(FIFO_NAME, S_IRUSR | S_IWUSR);
    arg.filedes = open(FIFO_NAME, O_RDONLY);
    // Setting up semaphores
    sem_t* sem_close = sem_open(SCLOSE, O_CREAT, S_IRUSR | S_IWUSR, 1);
    sem_t* sem_run = sem_open(SRUN, O_CREAT, S_IRUSR | S_IWUSR, 0);
    // perror(nullptr);
    arg.run_sem = sem_run;
    arg.close_sem = sem_close;
    if (pthread_create(&arg.thread_id, nullptr, thread_func, &arg)) {
        printf("Error opening first thread\n");
    }
    getchar();
    arg.thread_run = false;
    int *exit_code1;
    if (pthread_join(arg.thread_id, (void**)&exit_code1)) {
        printf("Error joining first thread\n");
    }
    close(arg.filedes);
    unlink(FIFO_NAME);
    sem_unlink(SRUN);
    sem_unlink(SUNLINK);
    return 0;
}