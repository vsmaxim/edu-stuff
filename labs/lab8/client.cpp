#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <pthread.h>
#include <queue>
#include <string>
#include <errno.h>
#define SOCKET_PATH "somesock"
#define CONNECTION_QUEUE_SIZE 20
#define BUFFER_LENGTH 40

using std::queue;
using std::string;

bool send_req_finish = false,
     get_resp_finish = false,
     con_est_finish = false;

pthread_t send_req_thread,
          get_resp_thread,
          con_est_thread;

struct socket_data {
    sockaddr_un addres = { AF_UNIX, SOCKET_PATH };
    int descriptor;
};

void flush_prints(char *pre, char *s) {
    fflush(stdout);
    printf("%s %s\n", pre, s);
}

static void *send_request(void *arg) {
    socket_data* data = (socket_data*)arg;
    while (!send_req_finish) {
        sleep(1);
        char request[] = "Request?";
        send(data->descriptor, request, strlen(request), 0);
	perror("send request");
    }
}

static void *get_response(void *arg) {
    socket_data* data = (socket_data*)arg;
    char buffer[BUFFER_LENGTH];
    buffer[BUFFER_LENGTH - 1] = 0;
    while (!get_resp_finish) {
        recv(data->descriptor, buffer, BUFFER_LENGTH - 1, 0);
        flush_prints("Response is: ", buffer);
	perror("get response");
    }
}

static void *connection_establish(void *arg) {
    socket_data *data = (socket_data*)arg;
    while (!con_est_finish) {
        int connection = connect(data->descriptor, (const sockaddr *)&data->addres, sizeof(sockaddr));
	perror("connection:");
        if (connection == 0) {
            printf("Connected to server\n");
            pthread_create(&send_req_thread, nullptr, send_request, arg);
	    perror("send thread opening");
            pthread_create(&get_resp_thread, nullptr, get_response, arg);
            perror("response thread opening");
	    pthread_exit(nullptr);
        } else {
            int err = errno;
            fflush(stdout);
            sleep(1);
        }
    }
} 

int main() {
    socket_data data;
    data.descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
    perror("creating socket");
    if (data.descriptor == -1) {
        printf("Error creating socket\n");
    };
    // Connection establishing thread
    pthread_create(&con_est_thread, nullptr, connection_establish, (void*)&data);
    perror("opening thread");
    getchar();
    con_est_finish = true;
    send_req_finish = true;
    get_resp_finish = true;
    pthread_join(con_est_thread, nullptr);
    pthread_join(get_resp_thread, nullptr);
    pthread_join(send_req_thread, nullptr);
    close(data.descriptor);
    return 0;
}
