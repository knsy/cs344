/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h> //for iscntrl()
#include <stdlib.h> //for random()

void dostuff(int); /* function prototype */
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

//returns the remainder of a devision.
//not sure why % couldn't be used
//but specifications called for this.
int modNum(int numIn, int modNum){
   while(numIn < 0){
      numIn += modNum;
   }
   while(numIn >= modNum){
      numIn -= modNum;
   }

   return numIn;
}

int main(int argc, char *argv[])
{	//setup
     int sockfd, newsockfd, portno, pid;
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;
	
	//cmd line error check
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     
	//new socket
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     
     //bind the socket to port that was passed in
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     
     int splitPort = portno;
     
     while (1) {
		 //accept new connection
         newsockfd = accept(sockfd, 
               (struct sockaddr *) &cli_addr, &clilen);
         if (newsockfd < 0) 
             error("ERROR on accept");
         
         ///printf("S:newcon Accepted");
         
         //setup new port listener/new socket
        splitPort++;
        
		int splitSockFD, newSplitSocFD;
		socklen_t splitClilen;
		struct sockaddr_in splitServ_addr, splitCli_addr;
     
         splitSockFD = socket(AF_INET, SOCK_STREAM, 0);
		 if (splitSockFD < 0) 
		 error("ERROR opening split socket");
		 bzero((char *) &splitServ_addr, sizeof(splitServ_addr));
		 splitServ_addr.sin_family = AF_INET;
		 splitServ_addr.sin_addr.s_addr = INADDR_ANY;
		 splitServ_addr.sin_port = htons(splitPort);
		 
		 //bind the socket to port that was passed in
		if (bind(splitSockFD, (struct sockaddr *) &splitServ_addr,
              sizeof(splitServ_addr)) < 0) 
              error("ERROR on split binding");
		listen(splitSockFD,1);
		splitClilen = sizeof(splitCli_addr);
         
         //send back the new port number if enc "received"
         //otherwise accept connection and drop it.
         int n;
		 char buffer[256];
			  
		 bzero(buffer,256);
		 n = read(newsockfd,buffer,255);
		 if (n < 0) error("ERROR reading from socket");
		 ///printf("S:Here is the message: %s\n",buffer);
		 
		 if(buffer[0] == 'd'){
			 char portConv[6];
			 sprintf(portConv, "%d\n", splitPort);
			 
			 //send new port
			 n = write(newsockfd,portConv, strlen(portConv) + 1);
			 if (n < 0) error("ERROR writing to socket");
			 
		 } else {
			 perror("S:Only otp_dec can connect to this service.");
			 
			 //send new port
			 n = write(newsockfd,"x", 1);
			 if (n < 0) error("ERROR writing to socket");
			 
			 continue; 
			 
			 //THIS FOLLOWING STUFF IS IRRELEVANT?
			 newSplitSocFD = accept(splitSockFD, 
               (struct sockaddr *) &splitCli_addr, &splitClilen);
				if (newSplitSocFD < 0) 
				error("ERROR on new accept");
			close(newSplitSocFD);
			
		 }
		 //accept new connection
         newSplitSocFD = accept(splitSockFD, 
               (struct sockaddr *) &splitCli_addr, &splitClilen);
         if (newSplitSocFD < 0) 
             error("ERROR on new accept");
         
         //split off a new process
         pid = fork();
         if (pid < 0)
             error("ERROR on fork");
         if (pid == 0)  {
			 //child that takes the new connection with it
             close(sockfd);
             close(splitSockFD);
             dostuff(newSplitSocFD);
             close(newSplitSocFD);
             exit(0);
         }
         //parent that closes the accepted connection and keeps listening.
         else {
			 close(newsockfd);
			 close(splitSockFD);
			 close(newSplitSocFD);
		 }
         
     } /* end of while */
     close(sockfd);
     return 0; /* we never get here */
}

/******** DOSTUFF() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void dostuff (int sock)
{
	char key[256];
	int n;
	char buffer[256];
	char cypher[256];
   
      
   /*bzero(buffer,256);
   n = read(sock,buffer,255);
   if (n < 0) error("ERROR reading from socket");
   printf("S:Here is the message to new port: %s\n",buffer);
   n = write(sock,"I got your message",18);
   if (n < 0) error("ERROR writing to socket"); */
   
   //receiving PLAINTEXT then KEY the  send off CYPHER
   int doneReading = 0;
   do{
	   //ERROR Add error check by length of shit taken in.
	   //read PLAINTEXT or 'd' for DONE...
	   bzero(buffer,256);
	   n = read(sock,buffer,255);
	   if (n < 0) error("ERROR reading from socket");
	   
	   ///printf("S: PLAIN?: %s", buffer);
	   
	   if(buffer[0] == 'd'){
		   ///printf("S: done? %s", buffer);
		   doneReading = 1;
	   }else{
		   
		strncpy(cypher, buffer, 256);
	   
	   //read section
	   bzero(buffer,256);
	   n = read(sock,buffer,255);
	   if (n < 0) error("ERROR reading from socket");
	   
	   ///printf("S: KEY?: %s", buffer);
	   
	   if(buffer[0] == 'd'){
		   ///printf("S: done? %s", buffer);
		   doneReading = 1;
	   }
	   
	   strncpy(key, buffer, 256);
		}	   
	   
	   //decode received data HERE//////////////////////////
	  //DECODE
		int i = 0;
		while(isprint(cypher[i])){
		   if(cypher[i] == 32){
			  cypher[i] = 91;
			  
		   }
		   if(key[i] == 32){
			  key[i] = 91;
		   }
		   int cypherChar = cypher[i] - 'A';
		   int keyChar = key[i] - 'A';

		   int decodeNum = cypherChar - keyChar;

		   int decodeChar = modNum(decodeNum, 27) + 'A';

		   if(decodeChar == 91){
			  decodeChar = 32;
		   }
		   
		   buffer[i]  = decodeChar;
		   i++;
		   //fprintf(stdout,"%c", decodeChar);
		   

		}
	   
	   ///printf("S: Encoded: %s\n", buffer);
	   
	   //send encoded data back
	   n = write(sock,buffer,strlen(buffer));
	   if (n < 0) error("ERROR reading from socket");
	   
	   bzero(key,256);
	   bzero(cypher,256);
   }
   while(!doneReading);
   
}
