#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct thread_args {
    bool thread_run;
    pthread_t thread_id;
    thread_args() : thread_run(true) {};
    int exit_code;
    int* filedes;
};

static void * first_thread(void *arg) {
    thread_args *args = (thread_args*) arg;
    printf("Started first thread\n");
    while (args->thread_run) {
        char a[] = "Hello world!\n";            
        write(args->filedes[1], a, strlen(a));
        sleep(1);
    }
    args->exit_code = 1;
    printf("Finished first thread\n");
    pthread_exit((void*)&args->exit_code);
}

static void * second_thread(void *arg) {
    thread_args *args = (thread_args*) arg;
    printf("Started second thread\n");
    char buf[15];
    while (args->thread_run) {
        buf[0] = '\0';
        read(args->filedes[0], buf, 14);
        printf("%s\n", buf);
    }
    args->exit_code = 2;
    printf("Finished second thread\n");
    pthread_exit((void*)&args->exit_code);
}

int main() {
    thread_args arg1, arg2;
    int fdes[2];
    pipe(fdes);
    arg1.filedes = fdes;
    arg2.filedes = fdes;
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
    close(fdes[0]);
    close(fdes[1]);
    printf("Exitcode 1 = %d\n", (int)*exit_code1);
    printf("Exitcode 2 = %d\n", (int)*exit_code2);
    return 0;
}