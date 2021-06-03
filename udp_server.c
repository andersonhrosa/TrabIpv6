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
   struct addrinfo hints, *srvinfo; //define as interfaces de entrada e saida para o preechimento com getaddrinfo

   char buf[INET6_ADDRSTRLEN]; //buffer para conversao do endereço em inet_ntop (network to print)

   struct sockaddr_in6 srv_addr; //define a estrutura para preenchimento de informações utilizada para envio de servidor
   struct sockaddr_in6 cli_addr; //define a estrutura para preenchimento de informações utilizada para recebimento enviado pelo cliente
   char    in[STATION_SIZE], out[TEMPERATURE_SIZE]; //define a cadeia de caracteres de entrada e saida do sensor de temperatura
   socklen_t client_length = sizeof(struct sockaddr_in); //define o tamanho da estrutura de recebimento enviada pelo cliente
   float   min = 0.0, max = 100.0; //utilizado em forecast_for como argumento da função
   char *hostname, *port; //utilizado para armazenar endereço ipv6 e porta definidos em commom.h
   int     sd, pid; //usado para o retorno do socket e na função fork
   int     optval = 1; //usado para setar informações do socket em setsockopt()
 
   bzero(&hints, sizeof(hints)); //seta para zero os bytes de hints
   
   hostname = SERVER_ADDR; //define o endereço do host
   port = FORECAST_PORT; //define a porta
   /*Seta informações para preechimento em getaddrinfo*/
   hints.ai_family = AF_INET6;    // AF_INET6 força o uso de IPv6
   hints.ai_socktype = SOCK_DGRAM; // define um socket udp 
   hints.ai_flags |= AI_CANONNAME; // usado para completar o ai_canonname com o nome real do host

   /* preecherá srvinfo com informações para uso na chamada do socket, utilizando
    * informações do hostname, port, e da estrutura hints.
    */
   if (getaddrinfo(hostname, port, &hints, &srvinfo) != 0) {
      perror("getaddrinfo failed");
      return(EXIT_FAILURE);
   }

   // Cria um socket IPv6/UDP 
   sd = socket(srvinfo->ai_family, srvinfo->ai_socktype,srvinfo->ai_protocol);
   if (sd < 0) {
      printf("Erro");
   }
   /*obtêm e define determinadas opções em um socket, neste caso poderá ser utilizado para reuso
    *Permite que outros sockets façam bind() contra esta porta, a
    *menos que haja um socket de escuta ativo já ligado à porta. Isso
    *permite que você contorne as mensagens de erro "Endereço já em
    *uso" ao tentar reiniciar o servidor após uma falha.
    */
   if (setsockopt(sd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char*)&optval, sizeof(optval)) < 0) {
      printf("setsockopt failed\n");
      close(sd);
      exit(2);
   }

   // Inicializa campo sin_zero em struct sockaddr_in como zero.
   memset(&srv_addr, 0, sizeof(srv_addr)); 
   srv_addr.sin6_family = AF_INET6; //define a familia de endereço para IPV6
   srv_addr.sin6_port = htons(atoi(FORECAST_PORT)); //define a porta convertendo a string para int e para formato de rede
   srv_addr.sin6_addr= in6addr_any; // Definido para no caso de haver multiplos hosts


   /* Associa uma porta ao socket a maquina local em srv_addr
    * O número da porta é usado pelo kernel para assciar um pacote
    * de entrada ao descritor de socket de um determinado processo
    */
   bind(sd, (struct sockaddr *)&srv_addr, sizeof(struct sockaddr_in6));
   // Lida com as solicitações (cada uma em seu próprio processo filho)
   do {
      /* Recebe a requisição do cliente usando o socket sd*/
      recvfrom(sd, in, STATION_SIZE, 0,
         (struct sockaddr *)&cli_addr, &client_length);
      // Cria o processo filho
      pid = fork();
      if (pid == 0) {
         // Envia a Resposta

      memset(&out, 0, TEMPERATURE_SIZE); //seta os bytes do buffer out para zero
      sprintf(out, "%5.1f", forecast_for(in, min, max)); //escreve em out o retorno da função de dados do sensor

      /*
       * Imprime a mensagem e o nome do cliente.
       * A porta é recebida na ordem de bytes da rede, então a traduzimos para
       * ordem de bytes do host antes de imprimi-lo.
       * O endereço de internet (IPv6) é recebido como 128 bits na ordem de bytes da rede
       * então usamos um utilitário que o converte em uma string impressa em
       * formato decimal pontilhado para legibilidade.
       */
      printf("Port assigned is %d", ntohs(srv_addr.sin6_port));
         /*
      * Imprime detalhes do cliente/peer e os dados recebidos
      * Recebe uma mensagem nos sockets em buf de tamanho máximo 32 de um cliente.
      * Como os dois últimos parâmetros não são nulos, o nome do cliente
      * serão colocados na estrutura de dados do cliente e no tamanho do cliente
      * será colocado no tamanho cli_addr_size.
      */
      printf("\nReceived message %s from domain %s internet address %s:%d\n", out,
                           (cli_addr.sin6_family == AF_INET6?"AF_INET6":"UNKNOWN"),
      inet_ntop(AF_INET6, &(cli_addr.sin6_addr),buf,INET6_ADDRSTRLEN), ntohs(cli_addr.sin6_port));

      /*Envia de volta as informações de temperatura para o cliente
       *usando o socket sd e a estrutura de endereço em cli_addr
       * */   
      sendto(sd, out, TEMPERATURE_SIZE, 0,
            (struct sockaddr *)&cli_addr, client_length);
      exit(0);
   }
      
    } while(1);
   /* Libera a memória alocada em getaddrinfo em srvinfo*/
   freeaddrinfo(srvinfo);  
   close(sd); //fecha o descritor de socket
}


void usage(void) {
     (void)fprintf(stderr, "usage: %s <IP_address> <portnumber>\n", __progname);
     exit(1);
}

