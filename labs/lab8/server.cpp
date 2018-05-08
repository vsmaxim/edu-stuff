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
};

struct connection_data {
	int client_desc;
	bool connection_closed = false;
	queue<char *> request_queue;
	queue<char *> response_queue;
	pthread_mutex_t resp_mut,
			req_mut;
	connection_data(int cd) {
		this->client_desc = cd;
		pthread_mutex_init(&resp_mut, nullptr);
		pthread_mutex_init(&req_mut, nullptr);
		pthread_mutex_lock(&req_mut);
		pthread_mutex_lock(&resp_mut);
	};
	~connection_data() {
		pthread_mutex_destroy(&resp_mut);
		pthread_mutex_destroy(&req_mut);
	};
				
};

void flush_prints(char *pre, char *s) {
    fflush(stdout);
    printf("%s %s\n", pre, s);
}

static void *get_request(void *arg) {
    connection_data *data = (connection_data*)arg;
    while (!get_request_finish && !data->connection_closed) {
        char buffer[BUFFER_LENGTH];
        buffer[BUFFER_LENGTH - 1] = 0;
        ssize_t message_size = recv(data->client_desc, buffer, BUFFER_LENGTH - 1, 0);
	data->connection_closed = (message_size == 0);
	flush_prints("Got request: ", buffer);
	// perror("get request");
	pthread_mutex_lock(&data->req_mut);
        data->request_queue.push(buffer);
	pthread_mutex_unlock(&data->req_mut);
	buffer[0] = 0;
	printf("In get request!\n");
    }
}

static void *process_request(void *arg) {
    connection_data *data = (connection_data*)arg;
    while (!process_request_finish && !data->connection_closed) {
	pthread_mutex_lock(&data->resp_mut);		
        if (!data->response_queue.empty()) {
		data->response_queue.push(data->request_queue.front());
	}
       	pthread_mutex_unlock(&data->resp_mut);
	pthread_mutex_lock(&data->req_mut);
	if (!data->request_queue.empty())
		data->request_queue.pop();
	pthread_mutex_unlock(&data->req_mut);
	printf("In process request\n");
    }
}

static void *send_response(void *arg) {
    sleep(1);
    connection_data *data = (connection_data*)arg;
    while (!send_response_finish && !data->connection_closed) {
        pthread_mutex_lock(&data->resp_mut);
        string response = data->response_queue.front();
	pthread_mutex_unlock(&data->resp_mut);
	if (response != nullptr) 
        send(data->client_desc, response.c_str(), response.length(), MSG_NOSIGNAL);
	perror("send response");
	printf("In send response!\n");
    }
}

void handle_connection(int connection_descriptor) {
	connection_data* cd = new connection_data(connection_descriptor);
        pthread_create(&get_req_thread, nullptr, get_request, (void*)cd);
        pthread_create(&proc_req_thread, nullptr, process_request, (void*)cd);
        pthread_create(&send_resp_thread, nullptr, send_response, (void*)cd);
};

static void *listen_socket(void *arg) {
    socket_data* data = (socket_data*)arg;
    printf("Listening for incoming connections\n");
    while (!listen_socket_finish) {
	printf("Waiting for another connections");
	socklen_t slen = sizeof(sockaddr);
        int head_connection = accept(data->descriptor, (sockaddr*)&data->addres, &slen);
	perror("accept connection");
	if (head_connection) {
		printf("Connection established!");
		handle_connection(head_connection);
	}
    }
    printf("listen_socket_finished");
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
