#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define arrlen(a) (sizeof (a) / sizeof (*(a)))

void print_sockaddr(const char *msg, struct sockaddr *sa)
{
    char ip[INET6_ADDRSTRLEN];

    printf("%s", msg);

    if (sa->sa_family == AF_INET) {
        struct sockaddr_in *sin = (struct sockaddr_in *) sa;

        printf("ipv4: %s; port: %d\n",
               inet_ntop(AF_INET, &sin->sin_addr, ip, arrlen(ip)),
               ntohs(sin->sin_port));

    } else if (sa->sa_family == AF_INET6) {
        struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) sa;

        printf("ipv6: %s; port: %d\n",
               inet_ntop(AF_INET6, &sin6->sin6_addr, ip, arrlen(ip)),
               ntohs(sin6->sin6_port));
    } else {
        printf("Unsupported address family: %d\n", sa->sa_family);
    }
}

void print_addrinfo(const char *msg, struct addrinfo *ai)
{
    printf("%s", msg);
    printf("family: %d; socktype: %d; protocol: %d; ",
           ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    print_sockaddr("", ai->ai_addr);
}

int main()
{
    int sockfd;
    int ret;
    char buffer[1024];
    ssize_t bytes;
    int yes = 1;

    struct addrinfo hints, *ai, *aii;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if ((ret = getaddrinfo("255.255.255.255", "9090", &hints, &ai)) == -1) {
        fprintf(stderr, "C: getaddrinfo() error: %s\n", gai_strerror(ret));
        return EXIT_FAILURE;
    }

    for (aii = ai; aii != NULL; aii = aii->ai_next) {
        print_addrinfo("C: Found address: ", aii);

        sockfd = socket(aii->ai_family, aii->ai_socktype, aii->ai_protocol);
        if (sockfd == -1) {
            fprintf(stderr, "C: socket() error: %s\n", strerror(errno));
            continue;
        }

        print_sockaddr("S: Selected: ", aii->ai_addr);
        break;
    }

    freeaddrinfo(ai);

    // Without the following setsockopt() call, the following error
    // occurs when sendto() is attempted to send a broadcast message.
    //
    // C: sendto() error: Permission denied
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &yes, sizeof yes) == -1) {
        fprintf(stderr, "C: setsockopt() error: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    if (aii == NULL) {
        fprintf(stderr, "C: error: could not connect to any address\n");
        return EXIT_FAILURE;
    }

    strcpy(buffer, "hello from client\n");
    bytes = sendto(sockfd, buffer, strlen(buffer), 0,
                   aii->ai_addr, aii->ai_addrlen);

    if (bytes == -1) {
        fprintf(stderr, "C: sendto() error: %s\n", strerror(errno));
        close(sockfd);
        return EXIT_FAILURE;
    } else {
        printf("C: sendto() %jd bytes: %.*s",
               (intmax_t) bytes, (int) bytes, buffer);
    }

    strcpy(buffer, "bye from client\n");
    bytes = sendto(sockfd, buffer, strlen(buffer), 0,
                   aii->ai_addr, aii->ai_addrlen);
    if (bytes == -1) {
        fprintf(stderr, "C: sendto() error: %s\n", strerror(errno));
        close(sockfd);
        return EXIT_FAILURE;
    } else {
        printf("C: sendto() %jd bytes: %.*s",
               (intmax_t) bytes, (int) bytes, buffer);
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
