#include "bbuffer.h"
#include <fcntl.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define handle_error(msg)                                                      \
    do {                                                                       \
        perror(msg);                                                           \
        exit(EXIT_FAILURE);                                                    \
    } while (0)
#define LISTEN_BACKLOG 50

typedef struct {
    BNDBUF *bb;
    int serve_dir;
} worker_state_t;

void write_header(int cfd, unsigned int code, unsigned int content_len) {
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
            "HTTP/1.1 %d %s\r\nContent-Length:%d\r\nContent-Type: "
            "text/html;charset=utf-8\r\nConnection: Closed\r\n\r\n",
            code, status, content_len);
    write(cfd, header, strlen(header));
}

static void *handle_request(void *worker_state) {
    worker_state_t *ws = worker_state;

    while (1) {
        int conn_socket = bb_get(ws->bb);

        char req_buf[64];
        int bytes_read = read(conn_socket, req_buf, sizeof(req_buf));

        if (bytes_read == -1) {
            handle_error("could not read request");
        }

        char *req = req_buf;
        char *req_line = strsep(&req, "\r\n");
        printf("Request Line: '%s'\n", req_line);
        char *req_method = strsep(&req_line, " ");

        if (strncmp("GET", req_method, 3) != 0) {
            char *response = "<html>Bad request</html>\r\n";
            if (write(conn_socket, response, strlen(response)) == -1) {
                handle_error("could not write response");
            }
        }

        char *file_path = strsep(&req_line, " \r\n");
        strsep(&file_path, "/");

        if (file_path[0] == '\0') {
            file_path = "index.html";
        }

        printf("File path: '%s'\n", file_path);

        int file_fd = openat(ws->serve_dir, file_path, O_RDONLY);
        if (file_fd == -1) {
            char *response = "<html><h1>404 Not Found!</h1></html>\r\n";
            write_header(conn_socket, 404, strlen(response));
            if (write(conn_socket, response, strlen(response)) == -1) {
                handle_error("could not write response");
            }
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

        shutdown(conn_socket, SHUT_WR);
        shutdown(conn_socket, SHUT_RD);
        close(conn_socket);
    }

    return ws;
}

int main(int argc, char *argv[]) { /*program name, www_path, port_number, number
                                      of threads and number of bufferslots*/
    if (argc != 5) {
        handle_error("too few arguments");
    }

    char *www_path = argv[1];
    in_port_t port = strtoul(argv[2], NULL, 10);
    unsigned long num_threads = strtoul(argv[3], NULL, 10);
    unsigned long num_bufferslots = strtoul(argv[4], NULL, 10);

    worker_state_t worker_state = {.bb = bb_init(num_bufferslots),
                                   .serve_dir = open(www_path, O_RDONLY)};

    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, &handle_request, &worker_state)) {
            handle_error("could not create thread");
        }
    }

    int sfd = socket(AF_INET, SOCK_STREAM, 0);

    struct in_addr addr = {
        .s_addr = INADDR_ANY /*any possible address */
    };

    printf("port: %d\n", port);
    struct sockaddr_in sockaddr = {.sin_family = AF_INET,
                                   .sin_port = __builtin_bswap16(port),
                                   .sin_addr = addr};

    if (bind(sfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) == -1) {
        handle_error("bind");
    }

    if (listen(sfd, LISTEN_BACKLOG) == -1) {
        handle_error("listen");
    }

    while (1) {
        struct sockaddr_in peer_addr; /*address for client */
        socklen_t peer_addrlen;
        int conn_socket =
            accept(sfd, (struct sockaddr *)&peer_addr, &peer_addrlen);

        if (conn_socket == -1) {
            handle_error("could not accept TCP connection");
        }

        bb_add(worker_state.bb, conn_socket);
    }

    close(sfd);
}
