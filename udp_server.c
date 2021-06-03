/*
  Compilar:
     clang -Wall udp_server.c weather.c -o udpserver
  Ref.:
     https://www.pucpcaldas.br/aularede/mod/page/view.php?id=88757
     https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.1.0/com.ibm.zos.v2r1.hala001/xsocudp.htm
     https://www.softlab.ntua.gr/facilities/documentation/unix/unix-socket-faq/unix-socket-faq-4.html
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include "common.h"
#include "weather.h"

int main(void) {
    struct addrinfo hints, *srvinfo;

   char buf[INET6_ADDRSTRLEN];

    struct sockaddr_in6 srv_addr;
    struct sockaddr_in6 cli_addr;
    char    in[STATION_SIZE], out[TEMPERATURE_SIZE];
    socklen_t client_length = sizeof(struct sockaddr_in);
    float   min = 0.0, max = 100.0;
    char *hostname, *port;
    int     sd, pid;
    int     optval = 1;
 
    bzero(&hints, sizeof(hints));
   //  if (daemon(1, 1) == -1) {
   //     perror("failed to become a daemon");
   //     exit(1);
   //  }
   
   hostname = SERVER_ADDR;
   port = FORECAST_PORT;

    hints.ai_family = AF_INET6;    // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags |= AI_CANONNAME;

    /* srvinfo must contain at least one addrinfo, use the first. */
    if (getaddrinfo(hostname, port, &hints, &srvinfo) != 0) {
        perror("getaddrinfo failed");
        return(EXIT_FAILURE);
     }

    // Create an IPv4/UDP socket
    sd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sd < 0) {
       printf("Erro");
    }

    if (setsockopt(sd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char*)&optval, sizeof(optval)) < 0) {
        printf("setsockopt failed\n");
        close(sd);
        exit(2);
    }

    // Initialize the address of this host
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin6_family = AF_INET6;
    srv_addr.sin6_port = htons(atoi(FORECAST_PORT));
    srv_addr.sin6_addr= in6addr_any; // In case the host has multiple


    // Bind the socket to the srv_addr
    bind(sd, (struct sockaddr *)&srv_addr, sizeof(struct sockaddr_in6));
    // Handle requests (each in its own child process)
    do {
       // Receive the request
       recvfrom(sd, in, STATION_SIZE, 0,
	       (struct sockaddr *)&cli_addr, &client_length);
       // Create the child process
       pid = fork();
       if (pid == 0) {
               // Close the child's copy of the incoming socket
            // close (sd);

               // Send the response
            memset(&out, 0, TEMPERATURE_SIZE);
            sprintf(out, "%5.1f", forecast_for(in, min, max));

               /*
                  * Print the message and the name of the client.
                  * The domain should be the internet domain (AF_INET) to IPv4.
                  * The port is received in network byte order, so we translate it to
                  * host byte order before printing it.
                  * The internet address (IPv4) is received as 32 bits in network byte order
                  * so we use a utility that converts it to a string printed in
                  * dotted decimal format for readability.
                  */
            printf("Port assigned is %d", ntohs(srv_addr.sin6_port));
                   /*
                  * Print details of the client/peer and the data received
                  * Receive a message on socket s in buf of maximum size 32 from a client.
                  * Because the last two paramters are not null, the name of the client
                  * will be placed into the client data structure and the size of the client
                  * address will be placed into cli_addr_size.
                  */
            printf("\nReceived message %s from domain %s internet address %s:%d\n", out,
                                 (cli_addr.sin6_family == AF_INET6?"AF_INET6":"UNKNOWN"),
            inet_ntop(AF_INET6, &(cli_addr.sin6_addr),buf,INET6_ADDRSTRLEN), ntohs(cli_addr.sin6_port));

            // sd = socket(AF_INET, SOCK_DGRAM, 0);
            sendto(sd, out, TEMPERATURE_SIZE, 0,
                  (struct sockaddr *)&cli_addr, client_length);
            // close(sd);
            exit(0);
         }
         
    } while(1);
    // /* Free answers after use */
   freeaddrinfo(srvinfo);  // All done with this structure
   close(sd);
}


void usage(void) {
     (void)fprintf(stderr, "usage: %s <IP_address> <portnumber>\n", __progname);
     exit(1);
}

