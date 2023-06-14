#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "my_socket.h"
#include "get_num.h"
#include "timespecop.h"

int debug = 0;

struct read_data {
    int read_bytes;
    struct timespec read_time;
    int rcvbuf_bytes;
};

int usage()
{
    char msg[] = "Usage: ./tcp-rcvbuf-growth [-d (debug)] [-b bufsize (4k)] [-n n_read (1000)] [-p port (1234)] [-r rcvbuf_size (default auto tuning)] remote_addr";
    fprintf(stderr, "%s\n", msg);

    return 0;
}

int main(int argc, char *argv[])
{
    int n_read = 1000;
    long bufsize = 4*1024;
    int port = 1234;
    int rcvbuf_size_by_hand = -1;

    int c;
    while ( (c = getopt(argc, argv, "b:dhn:p:r:")) != -1) {
        switch (c) {
            case 'b':
                bufsize = get_num(optarg);
                break;
            case 'd':
                debug = 1;
                break;
            case 'h':
                usage();
                exit(0);
            case 'n':
                n_read = strtol(optarg, NULL, 0);
                break;
            case 'p':
                port = strtol(optarg, NULL, 0);
                break;
            case 'r':
                rcvbuf_size_by_hand = get_num(optarg);
                break;
            default:
                break;
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 1) {
        usage();
        exit(1);
    }

    if (debug) {
        fprintf(stderr, "bufsize: %ld, n_read: %d, port: %d, remote: %s\n",
            bufsize, n_read, port, argv[0]);
    }

    char *buf = malloc(bufsize);
    if (buf == NULL) {
        err(1, "malloc for buf");
    }
    memset(buf, 0, bufsize);

    struct read_data *read_data = malloc(sizeof(struct read_data)*(n_read + 2));
    // +2: before connect() and just after connect()
    if (read_data == NULL) {
        err(1, "malloc for read_data");
    }
    memset(read_data, 0, sizeof(struct read_data)*(n_read + 1));

    char *remote = argv[0];
    int sockfd = tcp_socket();
    if (sockfd < 0) {
        errx(1, "tcp_socket");
    }

    if (rcvbuf_size_by_hand > 0) {
        if (set_so_rcvbuf(sockfd, rcvbuf_size_by_hand) < 0) {
            errx(1, "set_so_rcvbuf()");
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &read_data[0].read_time);
    read_data[0].read_bytes = 0;
    read_data[0].rcvbuf_bytes = get_so_rcvbuf(sockfd);

    if (connect_tcp(sockfd, remote, port) < 0) {
        errx(1, "connect_tcp");
    }

    clock_gettime(CLOCK_MONOTONIC, &read_data[1].read_time);
    read_data[1].read_bytes = 0;
    read_data[1].rcvbuf_bytes = get_so_rcvbuf(sockfd);

    for (int i = 2; i < n_read; ++i) { // i = 0: just after create tcp socket, i = 1: just after connect()
        int n = read(sockfd, buf, bufsize);
        clock_gettime(CLOCK_MONOTONIC, &read_data[i].read_time);
        read_data[i].read_bytes = n;
        read_data[i].rcvbuf_bytes = get_so_rcvbuf(sockfd);
    }

    int total_bytes = 0;
    for (int i = 0; i < n_read; ++i) {
        struct timespec elapsed;
        timespecsub(&read_data[i].read_time, &read_data[0].read_time, &elapsed);
        total_bytes += read_data[i].read_bytes;
        printf("%ld.%09ld %d %d %d %d\n", 
            elapsed.tv_sec, elapsed.tv_nsec,
            i,
            read_data[i].read_bytes, 
            read_data[i].rcvbuf_bytes,
            total_bytes
        );
    }
            
    return 0;
}
