#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "msg.h"
typedef struct msg message;
#define BUF 512
#define TRUE 1
void Usage(char *progname);
int LookupName(char *name, unsigned short port, struct sockaddr_storage *ret_addr, size_t *ret_addrlen);
int Connect(const struct sockaddr_storage *addr, const size_t addrlen, int *ret_fd);

void empty_buf(void)
{
    int num;
    do { num = getchar(); }
	 while ((num != '\n') && (num != EOF));
}

int main(int argc, char **argv) {
    if (argc != 3) {
        Usage(argv[0]);
    }

    unsigned short port = 0;
    if (sscanf(argv[2], "%hu", &port) != 1) {
        Usage(argv[0]);
    }

    struct sockaddr_storage addr;
    size_t addrlen;
    if (!LookupName(argv[1], port, &addr, &addrlen)) {
        Usage(argv[0]);
    }

    // Connect to the remote host.
    int socket_fd;
    if (!Connect(&addr, addrlen, &socket_fd)) {
        Usage(argv[0]);
    }
    char choice_arr[10];
    while (1) {
		printf ("Enter your choice (1 to put, 2 to get, 0 to quit): ");
		fgets(choice_arr, 2, stdin);
		int choice = atoi(choice_arr);
		message mesg;
        int wres;
		size_t buf_size = 127;
		char buffer[buf_size];
        char name[128];
		char *pbuffer = buffer;
        switch (choice) {
            case 1:
                mesg.type = PUT;
        		memset(buffer, '\0', buf_size);
                empty_buf();
				printf ("Enter the name: ");
				getline(&pbuffer, &buf_size, stdin);
				name[127] = '\0';
				buffer[strcspn(buffer, "\n")] = 0;
				strcpy(name, buffer);
                strcpy (mesg.rd.name, name);
                printf ("Enter the id: ");
                scanf ("%d", &mesg.rd.id);
                empty_buf();
				wres = write(socket_fd, &mesg, sizeof(mesg));
                if (wres == 0) {
                    printf("socket closed prematurely \n");
                    close(socket_fd);
                    return EXIT_FAILURE;
                }
                if (wres == -1) {
                    if (errno == EINTR)
                        continue;
                    printf("socket write failure \n");
                    close(socket_fd);
                    return EXIT_FAILURE;
                }
                if (wres == sizeof(mesg)) {
                    printf("Put success.\n");
                }
                break;
            case 2: 
                mesg.type = GET;
                printf ("Enter the id: ");
                scanf ("%d", &mesg.rd.id);
				strcpy(mesg.rd.name, "");
                wres = write(socket_fd, &mesg, sizeof(mesg));
                if (wres == 0) {
                    printf("socket closed prematurely \n");
                    close(socket_fd);
                    return EXIT_FAILURE;
                }
                if (wres == -1) {
                    if (errno == EINTR)
                        continue;
                    printf("Put failed.  \n");
                    close(socket_fd);
                    return EXIT_FAILURE;
                }
                if (wres == sizeof(mesg)) {
                    printf("Sent GET request successfully.\n");
                }

                // Will only read BUF-1 characters at most.
                char readbuf[BUF];
                int res;
                while (1) {
                    res = read(socket_fd, readbuf, BUF-1);
                    if (res == 0) {
                        printf("socket closed prematurely \n");
                        close(socket_fd);
                        return EXIT_FAILURE;
                    }
                    if (res == -1) {
                        if (errno == EINTR)
                            continue;
                        printf("Get failed. \n");
                        close(socket_fd);
                        return EXIT_FAILURE;
                    }
                    readbuf[res] = '\0';
                    if (strcmp (readbuf, "INVALID") == 0) {
                        printf ("The record does not exist in database  \n");
                        break;
                    }
                    else {
                    	message received_msg;
						memcpy(&received_msg, readbuf, sizeof(received_msg));
						printf ("name : %s\n", received_msg.rd.name);
						printf ("id : %d\n", received_msg.rd.id);
						empty_buf();
						break;
					}
                }
                break;
            case 0:
                printf ("Exiting client program ... \n");
                close(socket_fd);
                return EXIT_SUCCESS;
            default:
                printf ("Invalid choice... please enter 0 or 1 or 2 only \n");
        }
    }
}

void Usage(char *progname) {
    printf("usage: %s  hostname port \n", progname);
    exit(EXIT_FAILURE);
}

int LookupName(char *name, unsigned short port, struct sockaddr_storage *ret_addr, size_t *ret_addrlen) {
    struct addrinfo hints, *results;
    int retval;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // Do the lookup by invoking getaddrinfo().
    if ((retval = getaddrinfo(name, NULL, &hints, &results)) != 0) {
        printf( "getaddrinfo failed: %s", gai_strerror(retval));
        return 0;
    }

    // Set the port in the first result.
    if (results->ai_family == AF_INET) {
        struct sockaddr_in *v4addr = (struct sockaddr_in *) (results->ai_addr);
        v4addr->sin_port = htons(port);
    } else if (results->ai_family == AF_INET6) {
        struct sockaddr_in6 *v6addr = (struct sockaddr_in6 *)(results->ai_addr);
        v6addr->sin6_port = htons(port);
    } else {
        printf("getaddrinfo failed to provide an IPv4 or IPv6 address \n");
        freeaddrinfo(results);
        return 0;
    }

    // Return the first result.
    assert(results != NULL);
    memcpy(ret_addr, results->ai_addr, results->ai_addrlen);
    *ret_addrlen = results->ai_addrlen;

    // Clean up.
    freeaddrinfo(results);
    return 1;
}

int Connect(const struct sockaddr_storage *addr, const size_t addrlen, int *ret_fd) {
    // Create the socket.
    int socket_fd = socket(addr->ss_family, SOCK_STREAM, 0);
    if (socket_fd == -1) {
        printf("socket() failed: %s", strerror(errno));
        return 0;
    }

    // Connect the socket to the remote host.
    int res = connect(socket_fd,
                        (const struct sockaddr *)(addr),
                        addrlen);
    if (res == -1) {
        printf("connect() failed: %s", strerror(errno));
        return 0;
    }

    *ret_fd = socket_fd;
    return 1;
}
