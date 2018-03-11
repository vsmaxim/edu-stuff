#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <mqueue.h>
#define PTR_SIZE sizeof(int)
#define QNAME "queue"

struct thread_args {
    bool thread_run;
    pthread_t thread_id;
    thread_args() : thread_run(true) {};
    int exit_code;
    mqd_t mq;
};

static void * thread_func(void *arg) {
    thread_args *args = (thread_args*) arg;
    printf("Started first thread\n");
    char message[4];   
    while (args->thread_run) {
        message[0] = '\0';
        mq_receive(args->mq, message, 4, NULL);
        printf("Recieved: %s\n", message);
        sleep(1);
    }
    args->exit_code = 1;
    printf("Finished first thread\n");
    pthread_exit((void*)&args->exit_code);
}

int main() {
    thread_args arg;
    arg.mq = mq_open(QNAME, O_CREAT, S_IRUSR, NULL);
    if (pthread_create(&arg.thread_id, nullptr, thread_func, &arg)) {
        printf("Error opening first thread\n");
    }
    getchar();
    arg.thread_run = false;
    int *exit_code1;
    if (pthread_join(arg.thread_id, (void**)&exit_code1)) {
        printf("Error joining first thread\n");
    }
    mq_close(arg.mq);
    mq_unlink(QNAME);
    return 0;
}
