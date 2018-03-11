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
#define SOCKET_PATH "/tmp/some_socket.socket"
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
    sockaddr addres;
    int descriptor;
};

static void *send_request(void *arg) {
    socket_data* data = (socket_data*)arg;
    while (!send_req_finish) {
        sleep(1);
        char request[] = "Request?";
        send(data->descriptor, request, strlen(request), 0);
    }
}

static void *get_response(void *arg) {
    socket_data* data = (socket_data*)arg;
    char buffer[BUFFER_LENGTH];
    buffer[BUFFER_LENGTH - 1] = 0;
    while (!get_resp_finish) {
        if (recv(data->descriptor, buffer, BUFFER_LENGTH - 1, 0) != -1) {
        printf("Response is: %s", buffer);
        }
    }
}

static void *connection_establish(void *arg) {
    socket_data *data = (socket_data*)arg;
    while (!con_est_finish) {
        int connection = connect(data->descriptor, (const sockaddr *)&data->addres, sizeof(sockaddr));
        if (connection == 0) {
            printf("Connected to server\n");
            pthread_create(&send_req_thread, nullptr, send_request, arg);
            pthread_create(&get_resp_thread, nullptr, get_response, arg);
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
    if (data.descriptor == -1) {
        printf("Error creating socket\n");
    };
    // Set addres of server socket
    data.addres = { AF_UNIX, SOCKET_PATH };
    // Connection establishing thread
    pthread_create(&con_est_thread, nullptr, connection_establish, (void*)&data);
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