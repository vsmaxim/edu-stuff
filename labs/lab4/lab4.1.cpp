#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <semaphore.h>


struct thread_args {
    bool thread_run;
    pthread_t thread_id;
    thread_args() : thread_run(true) {};
    int exit_code;
    sem_t *sem;
    int filedes;
};

static void * thread_func(void *arg) {
    thread_args *args = (thread_args*) arg;
    printf("Started first thread\n");
    char a[] = "2";
    while (args->thread_run) {
        sem_wait(args->sem);
        for (int i = 0; i < 5; i++) {
            write(args->filedes, a, 1);
            sleep(1);
        }
        sem_post(args->sem);
        sleep(1);
    }
    args->exit_code = 1;
    printf("Finished first thread\n");
    pthread_exit((void*)&args->exit_code);
}

int main() {
    thread_args arg;
    sem_t* sem = sem_open("sem_2", O_CREAT, S_IRUSR | S_IWUSR, 1);
    arg.sem = sem;
    arg.filedes = open("output.txt", O_RDWR | O_CREAT | O_APPEND | O_NONBLOCK, S_IRUSR | S_IWUSR);
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
    sem_close(sem);
    sem_unlink("sem_2");
    return 0;
}