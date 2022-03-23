#include "bbuffer.h"
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define sock_sutdown(sfd)                                                      \
    shutdown(conn_socket, SHUT_WR);                                            \
    shutdown(conn_socket, SHUT_RD);                                            \
    close(conn_socket)
#define LISTEN_BACKLOG 50

typedef struct {
    BNDBUF *bb;
    int serve_dir;
} worker_state_t;

volatile sig_atomic_t running = 1;

void sighandler(int sig) { running = 0; }

// Helper function for writing a HTTP header
void write_header(int cfd, unsigned int code, size_t content_len) {
    char *header = malloc(128 * sizeof(char));

    char *status;
    switch (code) {
    case 404:
        status = "Not Found";
        break;
    default:
        status = "OK";
        break;
    }

    sprintf(header,
            "HTTP/1.1 %u %s\r\nContent-Length:%zu\r\nContent-Type: "
            "text/html;charset=utf-8\r\nConnection: Closed\r\n\r\n",
            code, status, content_len);
    if (write(cfd, header, strlen(header)) == -1) {
        perror("Could not write header");
    }

    free(header);
}

// Helper function for writing a HTTP response with a header and body
void write_response(int cfd, unsigned int code, char *body) {
    size_t bodylen = strlen(body);
    write_header(cfd, code, bodylen);

    if (write(cfd, body, bodylen) == -1) {
        perror("Could not write response body");
    }
}

// Worker thread function
static void *handle_request(void *worker_state) {
    worker_state_t *ws = worker_state;

    while (running) {
        int conn_socket = bb_get(ws->bb);

        char req_buf[64];
        int bytes_read = read(conn_socket, req_buf, sizeof(req_buf));
        // Ensure that the last byte is always a null character
        req_buf[63] = '\0';

        if (bytes_read == -1) {
            perror("Could not read request");
            sock_sutdown(conn_socket);
            continue;
        }

        char *req = req_buf;
        char *req_line = strsep(&req, "\r\n");
        printf("Request Line: '%s'\n", req_line);
        char *req_method = strsep(&req_line, " ");

        if (strncmp("GET", req_method, 3) != 0) {
            char *response = "<html><h1>400 Bad Request! Only GET requests are "
                             "supported.</h1></html>\r\n";
            write_response(conn_socket, 400, response);

            sock_sutdown(conn_socket);
            continue;
        }

        char *file_path = strsep(&req_line, " \r\n");

        // Reject requests that contain "../"
        if (strstr(file_path, "../") != NULL) {
            char *response = "<html><h1>400 Bad Request! Path cannot include "
                             "\"..\".</h1></html>\r\n";
            write_response(conn_socket, 400, response);

            sock_sutdown(conn_socket);
            continue;
        }

        // Remove leading forward slashes
        while (file_path[0] == '/') {
            strsep(&file_path, "/");
        }

        if (file_path[0] == '\0') {
            file_path = "index.html";
        }

        printf("File path: '%s'\n", file_path);

        int file_fd = openat(ws->serve_dir, file_path, O_RDONLY);
        if (file_fd == -1) {
            char *response = "<html><h1>404 Not Found!</h1></html>\r\n";
            write_response(conn_socket, 404, response);
        } else {
            struct stat stat;
            fstat(file_fd, &stat);
            write_header(conn_socket, 200, stat.st_size);

            FILE *file_stream = fdopen(file_fd, "r");
            int c;
            while ((c = fgetc(file_stream)) != EOF) {
                write(conn_socket, &c, 1);
            }

            close(file_fd);
        }

        sock_sutdown(conn_socket);
    }

    return ws;
}

int main(int argc, char *argv[]) {
    // Register a signal handler for SIGINT to gracefully shut down the server
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = sighandler;
    sigaction(SIGINT, &action, NULL);

    if (argc != 5) {
        fprintf(stderr, "%s: incorrect number of arguments (expected 4)\n",
                argv[0]);
        printf("\nusage: %s <www-path> <port> <threads> <bufferslots>\n",
               argv[0]);
        exit(EXIT_FAILURE);
    }

    char *www_path = argv[1];
    in_port_t port = strtoul(argv[2], NULL, 10);
    unsigned long num_threads = strtoul(argv[3], NULL, 10);
    unsigned long num_bufferslots = strtoul(argv[4], NULL, 10);

    worker_state_t worker_state = {.bb = bb_init(num_bufferslots),
                                   .serve_dir = open(www_path, O_RDONLY)};

    if (worker_state.bb == NULL) {
        fprintf(stderr, "Could not create ring buffer\n");
        exit(EXIT_FAILURE);
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, &handle_request, &worker_state)) {
            perror("Could not create thread");
            exit(EXIT_FAILURE);
        }
    }

    int sfd = socket(AF_INET6, SOCK_STREAM, 0);

    struct in6_addr addr = in6addr_any;

    struct sockaddr_in6 sockaddr = {.sin6_family = AF_INET6,
                                    .sin6_port = htons(port),
                                    .sin6_flowinfo = 0,
                                    .sin6_addr = addr,
                                    .sin6_scope_id = 0};

    if (bind(sfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1) {
        perror("Could not bind socket to address");
        exit(EXIT_FAILURE);
    }

    if (listen(sfd, LISTEN_BACKLOG) == -1) {
        perror("Could not listen to socket");
        exit(EXIT_FAILURE);
    }

    printf("Listening for requests on port %u...\n", port);

    while (running) {
        struct sockaddr_in peer_addr; /* address for client */
        socklen_t peer_addrlen;
        int conn_socket =
            accept(sfd, (struct sockaddr *)&peer_addr, &peer_addrlen);

        if (conn_socket == -1) {
            perror("Could not accept TCP connection");
            break;
        }

        bb_add(worker_state.bb, conn_socket);
    }

    printf("Shutting down server...\n");

    // Cancel worker threads and free resources
    for (int i = 0; i < num_threads; i++) {
        pthread_cancel(threads[i]);
    }

    free(threads);
    bb_del(worker_state.bb);
    close(sfd);
}
