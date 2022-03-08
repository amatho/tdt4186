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

int main(int argc, char *argv[]) { /*program name, www_path, port_number, number
                                      of threads and number of bufferslots*/
    if (argc != 5) {
        handle_error("too few arguments");
    }

    char *www_path = argv[1];
    in_port_t port = strtoul(argv[2], NULL, 10);
    unsigned long num_threads = strtoul(argv[3], NULL, 10);
    unsigned long num_bufferslots = strtoul(argv[4], NULL, 10);

    int sfd = socket(AF_INET, SOCK_STREAM, 0);

    struct in_addr addr = {
        .s_addr = INADDR_ANY /*any possible address */
    };

    printf("port: %d\n", port);
    struct sockaddr_in sockaddr = {.sin_family = AF_INET,
                                   .sin_port = __builtin_bswap16(port),
                                   .sin_addr = addr};

    if (bind(sfd, &sockaddr, sizeof(sockaddr)) == -1) {
        handle_error("bind");
    }

    if (listen(sfd, LISTEN_BACKLOG) == -1) {
        handle_error("listen");
    }

    int serve_dir = open(www_path, O_RDONLY);

    while (1) {
        struct sockaddr_in peer_addr; /*address for client */
        socklen_t peer_addrlen;
        int conn_socket = accept(sfd, &peer_addr, &peer_addrlen);

        if (conn_socket == -1) {
            handle_error("could not accept TCP connection");
        }

        char req_buf[64];
        int bytes_read = read(conn_socket, req_buf, sizeof(req_buf));
        if (bytes_read == -1) {
            handle_error("could not read request");
        }

        printf("Request: '%s'\n", req_buf);
        char *req = req_buf;
        char *req_type = strsep(&req, " ");
        if (strncmp("GET", req_type, 3) != 0) {
            char *response = "<html>Bad request</html>\r\n";
            if (write(conn_socket, response, strlen(response)) == -1) {
                handle_error("could not write response");
            }
        }

        char *file_path = strsep(&req, " \r\n");
        strsep(&file_path, "/");
        printf("File path: '%s'\n", file_path);

        int file_fd = openat(serve_dir, file_path, O_RDONLY);
        if (file_fd == -1) {
            char *response = "<html>404 Not Found</html>\r\n";
            if (write(conn_socket, response, strlen(response)) == -1) {
                handle_error("could not write response");
            }
        } else {
            struct stat file_stat;
            fstat(file_fd, &file_stat);
            char *file_buf = malloc(file_stat.st_size);
            read(file_fd, file_buf, file_stat.st_size);

            write(conn_socket, file_buf, file_stat.st_size);
            write(conn_socket, "\r\n", 2);

            free(file_buf);
            close(file_fd);
        }

        shutdown(conn_socket, SHUT_WR);
        shutdown(conn_socket, SHUT_RD);
        close(conn_socket);
    }

    close(sfd);
}
