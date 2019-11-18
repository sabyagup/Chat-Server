// ************************************ Client Code *********************************************
// Author        :      Sabyasachi Gupta (the.saby_grad17@tamu.edu)
// Organisation  :      Texas A&M University, CS for ECEN 602 Assignment 2
// Description   :      The client connects to the chat server socket and attempts to enter the
// 			chat room. Based on information received from the server, the client
// 			enters the chat room and lets the user chat with others or it waits
// 			to attempt to enter again.            
// Last_Modified :      10/13/2017


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <utils.h>



int main(int argc,char *argv[])
{
  int sockfd, conn_ret, n;
  char sendline[MAX_STR_LEN];
  int ret = 0;
  int port_number ;
  char *p0;
  long conv_arg_to_int = strtol(argv[3], &p0, 10);
  struct SBCP_Message_Format *SBCP_Message_to_Server;
  struct SBCP_Message_Format *SBCP_Message_from_Server;
  int i, len_ip, bytes_read;
  fd_set Master;                         
  fd_set Read_FDs;
  int select_return = 0;
  time_t begin;
  begin = time(NULL);
  int idle_flag = 0;
  struct addrinfo dynamic_addr, *ai, *p;
  int retv = 0;
  
  

  if (argc != 4){
    err_sys ("USAGE: ./client <Username> <IP_Address> <Port_Number>");
    return 0;
  }

  port_number = conv_arg_to_int;
  memset(&dynamic_addr, 0, sizeof dynamic_addr);
  dynamic_addr.ai_family = AF_UNSPEC;
  dynamic_addr.ai_socktype = SOCK_STREAM;
  

  if ((retv = getaddrinfo(argv[2], argv[3], &dynamic_addr, &ai)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retv));
      return 1;
  }

  for(p = ai; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
        err_sys("CLIENT: socket");
        continue;
    }
    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
        close(sockfd);
        err_sys("CLIENT: connect");
        continue;
    }
    break;
  }

  if (p == NULL) {
    fprintf(stderr, "CLIENT: failed to connect\n");
    return 2;
  }
  freeaddrinfo(ai); 


  // Building the SBCP message
  SBCP_Message_to_Server = malloc(sizeof(struct SBCP_Message_Format));
  SBCP_Message_to_Server->Version = 3;                                 // SBCP Message Version indicator 
  SBCP_Message_to_Server->Type = 2;                                    // SBCP Message type indicating JOIN 
  SBCP_Message_to_Server->Length = 24;                                 // SBCP Message containing username length field 
  SBCP_Message_to_Server->SBCP_Attributes.Type = 2;                    // Attr Type indicating payload containing username
  SBCP_Message_to_Server->SBCP_Attributes.Length = 20;                 // Attr Length indicating length of payload containing username
  strcpy(SBCP_Message_to_Server->SBCP_Attributes.Payload, argv[1]);    // Copying username to payload
  printf ("CLIENT: Attempting to JOIN chat room \n");
  if (write(sockfd, SBCP_Message_to_Server, sizeof(struct SBCP_Message_Format)) == -1) err_sys ("ERR: Write Error");
  FD_SET(0, &Read_FDs);              // add user input from keyboard to the Read_FDs set
  FD_SET(sockfd, &Read_FDs);         // add sockfd to the Read_FDs set
  while (1) {
    select_return = select (sockfd+1, &Read_FDs, NULL, NULL, NULL);
    if (select_return == -1) {
      err_sys("ERR: Select Error");
      exit(6);
    }
    if ((difftime (time(NULL), begin)) > 10 && idle_flag == 0) {                             // Occurence of timeout              // IDLE Bonus Feature
      begin = time(NULL);
      idle_flag = 1;                                                                         // Make user IDLE
      SBCP_Message_to_Server = malloc(sizeof(struct SBCP_Message_Format));
      SBCP_Message_to_Server->Version = 3;                                 // SBCP Message version indicator 
      SBCP_Message_to_Server->Type = 9;                                    // SBCP Message type indicating IDLE
      if (write(sockfd, SBCP_Message_to_Server, sizeof(struct SBCP_Message_Format)) == -1) err_sys ("ERR: Write Error");
    }
    for (i=0; i<=sockfd; i++) {
      if (FD_ISSET(i, &Read_FDs)){
	if (i == 0) {                                                           // Data to be read from keyboard of user
	   bzero(sendline, MAX_STR_LEN);
           fgets(sendline,MAX_STR_LEN,stdin);
	   len_ip = strlen(sendline) - 1;
	   if(sendline[len_ip] == '\n') sendline[len_ip] = '\0';                // Provision to detect unceremonious exit from client 
           SBCP_Message_to_Server = malloc(sizeof(struct SBCP_Message_Format));
           strcpy(SBCP_Message_to_Server->SBCP_Attributes.Payload, sendline);
           SBCP_Message_to_Server->Version = 3;                                 // SBCP Message version indicator 
           SBCP_Message_to_Server->Type = 4;                                    // SBCP Message type indicating SEND
           SBCP_Message_to_Server->Length = MAX_STR_LEN + 8;                    // SBCP Message containing username length field 
           SBCP_Message_to_Server->SBCP_Attributes.Type = 4;                    // Attr type indicating payload containing message
           SBCP_Message_to_Server->SBCP_Attributes.Length = MAX_STR_LEN + 12;   // Attr Length indicating length of payload containing message from user
           if (write(sockfd, SBCP_Message_to_Server, sizeof(struct SBCP_Message_Format)) == -1) err_sys ("ERR: Write Error");
	   begin = time(NULL);
	   idle_flag = 0;                                                             // User is not IDLE
	}		
	if (i == sockfd) {                                                      // Server data to be read
          SBCP_Message_from_Server = malloc(sizeof(struct SBCP_Message_Format));
          bytes_read = read(sockfd, SBCP_Message_from_Server, sizeof(struct SBCP_Message_Format));
          printf ("%s \n", SBCP_Message_from_Server->SBCP_Attributes.Payload);
          free(SBCP_Message_from_Server);
          if(SBCP_Message_from_Server->Type == 5 && SBCP_Message_from_Server->SBCP_Attributes.Type == 1)      // NACK bonus feature
            exit(7);              // Try again later
        }     
      }
      FD_SET (0, &Read_FDs);
      FD_SET (sockfd, &Read_FDs);
    }
  }
  close(sockfd);
  return 0;
}












