/*
 * Compilar com:
 *      clang -Wall udp_client.c -o udpclient
 * Sintaxe:
 *      ./udp_client or
 *      ./udp_client remote_host udp_port
 */
#include <stdio.h>
#include <string.h>     // For memset()
#include <stdlib.h>
#include <arpa/inet.h>  // For byte order conversions
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include "common.h"
#include "weather.h"

int main(int argc, char *argv[]) {
    struct addrinfo hints, *res, *ressave;
    struct sockaddr *sa;
    socklen_t salen;
    char *host, *port;

    char    in[TEMPERATURE_SIZE];
    char    out[] = "PUC";
    int     rc, sd, ch;

    while ((ch = getopt(argc, argv, "s")) != -1) {
          switch (ch) {
             case 's':
                 strncpy(out, argv[1], STATION_SIZE);
                 break;
             default:
                 usage();
          }
    }
    argc -= optind;
    argv += optind;

    if (argc < 2) {
        host = SERVER_ADDR;
        port = FORECAST_PORT;	// htons(FORECAST_PORT);
    } else if (argc == 2) {
        host = argv[0];
        port = argv[1];
    } else {
        usage();
    }

    /* Initilize addrinfo structure */
    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family  = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    if ((ch = getaddrinfo(host, port, &hints, &res)) != 0) {
        printf("%s error for %s, %s: %s", __progname, host, port, gai_strerror(ch));
    }
    ressave = res;

    do {	/* Each of the returned IP address is tried to reate an IPv?/UDP socket */
        sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sd >= 0) {
            break;	/* Success */
        }
    } while ((res=res->ai_next) != NULL);

    sa = malloc(res->ai_addrlen);
    memcpy(sa, res->ai_addr, res->ai_addrlen);
    salen = res->ai_addrlen;

    freeaddrinfo(ressave);

    do {
       // Send the request
       if ((rc = sendto(sd, out, STATION_SIZE, 0, sa, salen)) < 0) {
           /* Buffers aren't available locally at the moment, try again. */
	   if (errno == ENOBUFS) {
               continue;
           }
           perror("sending datagram");
	   exit(1);
       }
    } while (errno == ENOBUFS);

    /* Read acknowledgement from remote system */
    if (recvfrom(sd, in, TEMPERATURE_SIZE, 0, NULL, NULL) < 0) {
	printf("server error: errno %d\n", errno);
	perror("reading datagram");
	exit(1);
    }
    // Display the response
    write(STDOUT_FILENO, "Value: ", 7);
    write(STDOUT_FILENO, in, TEMPERATURE_SIZE);
    write(STDOUT_FILENO, "\n", 1);

    close(sd);
}

void usage(void) {
     (void)fprintf(stderr, "usage: %s [-s] <hostname/IP_address> <portnumber>\n", __progname);
     exit(1);
}


