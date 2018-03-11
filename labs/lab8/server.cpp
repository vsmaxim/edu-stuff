#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <pthread.h>
#include <queue>
#include <string>
#define SOCKET_PATH "/tmp/some_socket.socket"
#define CONNECTION_QUEUE_SIZE 20
#define BUFFER_LENGTH 40

using std::queue;
using std::string;

bool get_request_finish = false,
     process_request_finish = false,
     send_response_finish = false,
     listen_socket_finish = false;

pthread_t get_req_thread,
          proc_req_thread, 
          send_resp_thread, 
          list_sock_thread;    

struct socket_data {
    sockaddr_un addres;
    int descriptor;
    queue<string> request_queue;
    queue<string> response_queue;
};

static void *get_request(void *arg) {
    socket_data *data = (socket_data*)arg;
    while (!get_request_finish) {
        char buffer[BUFFER_LENGTH];
        buffer[BUFFER_LENGTH - 1] = 0;
        ssize_t message_size = recv(data->descriptor, buffer, BUFFER_LENGTH - 1, 0);
        data->request_queue.push(buffer);
    }
}

static void *process_request(void *arg) {
    socket_data *data = (socket_data*)arg;
    while (!process_request_finish) {
        while (data->request_queue.empty()) {
            sleep(1);
        }
        data->response_queue.push(data->request_queue.front());
        data->request_queue.pop();
    }
}

static void *send_response(void *arg) {
    socket_data *data = (socket_data*)arg;
    while (!send_response_finish) {
        while(data->response_queue.empty()) {
            sleep(1);
        }
        string response = data->response_queue.front();
        data->response_queue.pop();
        send(data->descriptor, response.c_str(), response.length(), 0);
    }
}

static void *listen_socket(void *arg) {
    socket_data* data = (socket_data*)arg;
    printf("Listening for incoming connections\n");
    while (!listen_socket_finish) {
        int head_connection = accept(data->descriptor, NULL, NULL);
        if (head_connection) {
            printf("Connection established!");
            // Connection established
            pthread_create(&get_req_thread, nullptr, get_request, (void*)data);
            pthread_create(&proc_req_thread, nullptr, process_request, (void*)data);
            pthread_create(&send_resp_thread, nullptr, send_response, (void*)data);
            pthread_exit(nullptr);
        }
    }
}

int main() {
    socket_data data;
    data.descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
    if (data.descriptor == -1) {
        printf("Error creating socket\n");
    };
    // Nullify name for compatibility
    memset(&data.addres, 0, sizeof(data.addres));
    data.addres.sun_family = AF_UNIX;
    strncpy(data.addres.sun_path, SOCKET_PATH, sizeof(SOCKET_PATH) - 1);
    // Unlink if socket wasn't unlinked last time
    unlink(SOCKET_PATH);
    // Bind socket
    if (bind(data.descriptor, (const sockaddr *)&data.addres, sizeof(sockaddr_un))) {
        printf("Error binding socket\n");
    };
    if (listen(data.descriptor, CONNECTION_QUEUE_SIZE)) {
        printf("Error listening socket\n");   
    };
    // Create request and response queues
    if (pthread_create(&list_sock_thread, nullptr, listen_socket, (void*)&data)) {
        printf("Error creating thread\n");   
    };
    getchar();
    // Finish threads
    get_request_finish = true;
    process_request_finish = true;
    send_response_finish = true;
    listen_socket_finish = true;
    // Join threads
    pthread_join(get_req_thread, nullptr);
    pthread_join(proc_req_thread, nullptr);
    pthread_join(send_resp_thread, nullptr);
    pthread_join(list_sock_thread, nullptr);
    // Close socket
    close(data.descriptor);
    return 0;
}