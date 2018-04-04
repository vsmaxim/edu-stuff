#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <pthread.h>
#include <queue>
#include <string>
#define SOCKET_PATH "somesock"
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
    sockaddr_un addres = { AF_UNIX, SOCKET_PATH };
    int descriptor;
    int client_desc;
    queue<char*> request_queue;
    queue<char*> response_queue;
};

void flush_prints(char *pre, char *s) {
    fflush(stdout);
    printf("%s %s\n", pre, s);
}

static void *get_request(void *arg) {
    socket_data *data = (socket_data*)arg;
    while (!get_request_finish) {
        char buffer[BUFFER_LENGTH];
        buffer[BUFFER_LENGTH - 1] = 0;
        ssize_t message_size = recv(data->client_desc, buffer, BUFFER_LENGTH - 1, 0);
	flush_prints("Got request: ", buffer);
	// perror("get request");
	sleep(1);
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
    sleep(1);
    socket_data *data = (socket_data*)arg;
    while (!send_response_finish) {
        while(data->response_queue.empty()) {
            sleep(1);
        }
        string response = data->response_queue.front();
        data->response_queue.pop();
        send(data->client_desc, response.c_str(), response.length(), 0);
	// perror("send response");
    }
}

static void *listen_socket(void *arg) {
    socket_data* data = (socket_data*)arg;
    printf("Listening for incoming connections\n");
    while (!listen_socket_finish) {
	socklen_t slen = sizeof(sockaddr);
        int head_connection = accept(data->descriptor, (sockaddr*)&data->addres, &slen);
	perror("accept connection");
        if (head_connection) {
            printf("Connection established!");
	    data->client_desc = head_connection;
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
    unlink(SOCKET_PATH);
    data.descriptor = socket(AF_UNIX, SOCK_STREAM, 0);
    perror("creating socket");
    if (data.descriptor == -1) {
        printf("Error creating socket\n");
    };
    // Unlink if socket wasn't unlinked last time
    // Bind socket
    bind(data.descriptor, (const sockaddr *)&data.addres, sizeof(sockaddr_un));
    perror("binding");
    listen(data.descriptor, CONNECTION_QUEUE_SIZE);
    perror("listening");
    // Create request and response queues
    pthread_create(&list_sock_thread, nullptr, listen_socket, (void*)&data);
    perror("thread");
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
