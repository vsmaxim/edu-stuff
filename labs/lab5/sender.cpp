#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#define PTR_SIZE sizeof(int)
#define SREAD "sem_read"
#define SWRITE "sem_write"
#define SCLOSE "sem_close"
#define SRUN "sem_run"

struct thread_args {
    bool thread_run;
    pthread_t thread_id;
    thread_args() : thread_run(true) {};
    int exit_code;
    sem_t *read_sem;
    sem_t *write_sem;
    sem_t *close_sem;
    sem_t *run_sem;
    int filedes;
    int *ptr;
};

static void * thread_func(void *arg) {
    thread_args *args = (thread_args*) arg;
    int value;
    int exit_code;
    int pr_running = 1;
    printf("Started first thread\n");
    
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
        // Thread job
        value = rand();
        printf("Random value is: %d\n", value);
        *args->ptr = value;
        sem_post(args->write_sem);
        sem_wait(args->read_sem);
        sleep(1);
    }
    if (exit_code) sem_trywait(args->close_sem);
    args->exit_code = 1;
    printf("Finished first thread\n");
    pthread_exit((void*)&args->exit_code);
}

int main() {
    thread_args arg;
    //unlink 
    //sem_unlink(SREAD);
    //sem_unlink(SWRITE);
    //sem_unlink(SRUN);
    //sem_unlink(SCLOSE);
    // Opening semaphores for read, write and close sync
    sem_t* sem_read = sem_open(SREAD, O_CREAT, S_IRUSR | S_IWUSR, 0);
    sem_t* sem_write = sem_open(SWRITE, O_CREAT, S_IRUSR | S_IWUSR, 0);
    sem_t* sem_close = sem_open(SCLOSE, O_CREAT, S_IRUSR | S_IWUSR, 1);
    sem_t* sem_run = sem_open(SRUN, O_CREAT, S_IRUSR | S_IWUSR, 0);
    perror(nullptr);
    arg.read_sem = sem_read;
    arg.write_sem = sem_write;
    arg.close_sem = sem_close;
    arg.run_sem = sem_run;
    // Opening shared mem
    arg.filedes = shm_open("shared_mem.txt", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(arg.filedes, PTR_SIZE);
    arg.ptr = (int*)mmap(
        NULL,
        PTR_SIZE, 
        PROT_READ | PROT_WRITE, 
        MAP_SHARED, 
        arg.filedes, 
        0
    );
    if (pthread_create(&arg.thread_id, nullptr, thread_func, &arg)) {
        printf("Error opening first thread\n");
    }
    getchar();
    arg.thread_run = false;
    int *exit_code1;
    if (pthread_join(arg.thread_id, (void**)&exit_code1)) {
        printf("Error joining first thread\n");
    }
    munmap((void*)arg.ptr, PTR_SIZE);
    close(arg.filedes);
    shm_unlink("shared_mem.txt");
    sem_unlink(SREAD);
    sem_unlink(SWRITE);
    sem_unlink(SRUN);
    sem_unlink(SCLOSE);
    return 0;
}
