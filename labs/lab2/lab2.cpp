#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>


struct thread_args {
    bool thread_run;
    pthread_t thread_id;
    pthread_mutex_t *mutex;
    thread_args() : thread_run(true) {};
    int exit_code;
};

static void * first_thread(void *arg) {
    thread_args *args = (thread_args*) arg;
    printf("Started first thread\n");
    while (args->thread_run) {
        pthread_mutex_lock(args->mutex);
        for (int i = 0; i < 5; i++){
            printf("1");
            fflush(stdout);
            sleep(1);
        }
        pthread_mutex_unlock(args->mutex);
        sleep(1);
    }
    args->exit_code = 1;
    printf("Finished first thread\n");
    pthread_exit((void*)&args->exit_code);
}

static void * second_thread(void *arg) {
    thread_args *args = (thread_args*) arg;
    printf("Started second thread\n");
    while (args->thread_run) {
        pthread_mutex_lock(args->mutex);
        for (int i = 0; i < 5; i++){
            printf("2");
            fflush(stdout);
            sleep(1);
        }
        pthread_mutex_unlock(args->mutex);
        sleep(1);
    }
    args->exit_code = 2;
    printf("Finished second thread\n");
    pthread_exit((void*)&args->exit_code);
}

int main() {
    thread_args arg1, arg2;
    pthread_mutex_t mut;
    pthread_mutex_init(&mut, nullptr);
    arg1.mutex = &mut;
    arg2.mutex = &mut;
    if (pthread_create(&arg1.thread_id, nullptr, first_thread, &arg1)) {
        printf("Error opening first thread\n");
    }
    if (pthread_create(&arg2.thread_id, nullptr, second_thread, &arg2)) {
        printf("Error opening second thread\n");
    }
    getchar();
    arg1.thread_run = false;
    arg2.thread_run = false;
    int *exit_code1, *exit_code2;
    if (pthread_join(arg1.thread_id, (void**)&exit_code1)) {
        printf("Error joining first thread\n");
    }
    if (pthread_join(arg2.thread_id, (void**)&exit_code2)) {
        printf("Error joining second thread\n");
    }
    pthread_mutex_destroy(&mut);
    printf("Exitcode 1 = %d\n", (int)*exit_code1);
    printf("Exitcode 2 = %d\n", (int)*exit_code2);
    return 0;
}