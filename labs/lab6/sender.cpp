#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
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
    pthread_t thread_id;
    int exit_code;
    int filedes;
    sem_t* run_sem, *close_sem;
};

bool thread_run = true; 

void signal_handler(int signal) {
	if (signal == SIGPIPE) {
		thread_run = false;
		printf("Another proccess was accidently closed, finishing...\n");
	}
};

static void * thread_func(void *arg) {
    thread_args *args = (thread_args*) arg;
    printf("Started first thread\n");
    char message[] = "Hi!";
    signal(SIGPIPE, signal_handler);
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
    while (thread_run) {
        // Sync exiting
        write(args->filedes, message, 3);
	if (errno == EPIPE) {
		fflush(stdout);
		printf("Another process finished, exiting...\n");
		break;
	}
        printf("Sent: %s\n", message);
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
    if (sem_value) {
        sem_wait(s_unlink);
        unlink(FIFO_NAME);
        unlink(SRUN);
        unlink(SCLOSE);
    }
}

int main() {
    thread_args arg;
    unlink_old();
    // Synchronized unlinking old fifo and sems
    mkfifo("fifo", S_IRUSR | S_IWUSR);
    arg.filedes = open("fifo", O_WRONLY);
    sem_t* sem_close = sem_open(SCLOSE, O_CREAT, S_IRUSR | S_IWUSR, 1);
    sem_t* sem_run = sem_open(SRUN, O_CREAT, S_IRUSR | S_IWUSR, 0);
    arg.close_sem = sem_close;
    arg.run_sem = sem_run;
    if (pthread_create(&arg.thread_id, nullptr, thread_func, &arg)) {
        printf("Error opening first thread\n");
    }
    getchar();
    thread_run = false;
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
