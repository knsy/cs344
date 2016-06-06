/********************************************************
 * otp_dec_d.c is a server that accpets connections from
 * otp_dec and forks them off into new processes on a new port
 * which then receives a KEY and CYPHERTEXT that are then combined
 * into PLAINTEXT and returned to the client.
 * USE: otp_dec_d PORT
 * EXAMPLE: otp_dec_d 35555 &
 * WRITTEN BY: Konstantin Yakovenko
 * Based on the sample client/server from: 
 * http://www.linuxhowtos.org/C_C++/socket.htm
 * ******************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h> //for iscntrl()
#include <stdlib.h> //for random()

void processDecCon(int); /* function prototype */
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

//returns the remainder of a devision. MOD
//not quite sure why % couldn't be used
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
{	//setup variables
     int sockfd, newsockfd, portno, pid;
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;
	
	//check that the number of arguments is proper
     if (argc < 2) {
         fprintf(stderr,"Proper Usage: %s PORT \n", argv[0]);
         exit(1);
     }
     
	//create new socket
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) {
		fprintf(stderr,"ERROR opening socket\n");
        exit(1);
     }
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]); //grab the port that was passed in
     
     //fill in the server info 
     serv_addr.sin_family = AF_INET;  //TCP
     serv_addr.sin_addr.s_addr = INADDR_ANY;  //accept any address
     serv_addr.sin_port = htons(portno); //htons() converts to network bit order
     
     //bind the socket to port that was passed in
     if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){ 
            fprintf(stderr,"ERROR on binding\n");
          	exit(1);
     }
     
     //start listening to the socket for up to 5 connections
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     
     //here we will create a new port for moving our
     //connection off onto to keep the main server
     //free to accept additional connections
     int splitPort = portno;
     
     while (1) {
		 //accept new connection
         newsockfd = accept(sockfd, 
               (struct sockaddr *) &cli_addr, &clilen);
         if (newsockfd < 0) {
             fprintf(stderr,"ERROR on accepting connection\n");
             continue;
		 }
         
         //setup new port listener/new socket
        splitPort++;
        
        //variables for the new connection
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
		 //if the port is taken, we move to the next one
         //Ideally, this should be limited within some range of ports.
         //but since we are not hardcoding the ports, I am not sure how to
         //limit it in a meaningful way... This reasonably works right now.
         //but should be fixed.
		while(bind(splitSockFD, (struct sockaddr *) &splitServ_addr,
              sizeof(splitServ_addr)) < 0) {
			///fprintf(stderr,"ERROR Split Binding... Trying Another port...\n");
			splitPort++;
			splitServ_addr.sin_port = htons(splitPort);	  
		}
		
		
		listen(splitSockFD,1);
		splitClilen = sizeof(splitCli_addr);
         
         //send back the new port number if enc "received"
         //otherwise accept connection and drop it.
         //"dec" is short for "decode" and is used to make sure
         //that only otp_dec connects and not opt_enc
         int n;
         
         //somewhat arbitrary size. Large enough to satisfy our speed
         //requirements, doesn't go over int size.
		 char buffer[25001];
			  
		 bzero(buffer,25001);
		 n = read(newsockfd,buffer,25000);
		 if (n < 0) error("ERROR reading from socket");
		 ///printf("S:Here is the message: %s\n",buffer);
		 
		 //here we check for "dec"... although we only check for 'd'
		 //not sure if there are benefits to checking the whole 
		 //message or if one char is enough. Seemed faster than
		 //calling a string comparing function.
		 if(buffer[0] == 'd'){
			 char portConv[6];
			 sprintf(portConv, "%d\n", splitPort);
			 
			 //if 'authentication' goes through send new port
			 n = write(newsockfd,portConv, strlen(portConv) + 1);
			 if (n < 0) error("ERROR writing to socket");
			 
		 } else {
			 //if 'authentication' fails, we send an 'x' back
			 //telling the client that it is dunk and trying to connect
			 //to the wrong process and restart the while loop
			 ///perror("S:Only otp_dec can connect to this service.");

			 n = write(newsockfd,"x", 1);
			 if (n < 0) error("ERROR writing to socket");
			 
			 continue; 
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
			 //child takes the new connection with it and closes 
			 //connections to the main server's port.
             close(sockfd);
             close(splitSockFD);
             processDecCon(newSplitSocFD);
             close(newSplitSocFD);
             exit(0);
         }
         //parent closes the accepted connection and keeps listening
         //on the main port, taking any pending waiting connections
         //through this process again.
         else {
			 close(newsockfd);
			 close(splitSockFD);
			 close(newSplitSocFD);
		 }
         
     }
     
     //some compilers want a return for main
     //but we should never get here. EVER.
     close(sockfd);
     return 0;
}

//this is where tha main communication and decoding happens
void processDecCon (int sock)
{
	char key[25001];
	int n;
	char buffer[25001];
	char cypher[25001];
   
   //receiving CYPHER then KEY the  send off PLAINTEXT
   int doneReading = 0;
   do{
	   //ERROR Add error check by length of shit taken in.
	   //read CYPHER or 'd' for DONE...
	   bzero(buffer,25001);
	   n = read(sock,buffer,25000);
	   if (n < 0) error("ERROR reading from socket");
	   
	   ///printf("S: PLAIN?: %s", buffer);
	   
	   //if the message consists of 'd' for 'done'
	   //we slowly start winding down this session
	   if(buffer[0] == 'd'){
		   ///printf("S: done? %s", buffer);
		   doneReading = 1;
	   }else{
		//put received CYPHER from buffer[] into cypher[]
		//for processing    
		strncpy(cypher, buffer, 25001);
	   
	   //read KEY
	   bzero(buffer,25001);
	   n = read(sock,buffer,25000);
	   if (n < 0) error("ERROR reading from socket");
	   
	   ///printf("S: KEY?: %s", buffer);
	   
	   //if the message consists of 'd' for 'done'
	   //we slowly start winding down this session
	   if(buffer[0] == 'd'){
		   ///printf("S: done? %s", buffer);
		   doneReading = 1;
	   }
	   //put received KEY from buffer[] into key[]
	   //for processing 
	   strncpy(key, buffer, 25001);
		}	   
	   
	   //DECODE CYPHER + KEY//////////////////////////
	   //loop through chars, convert to num
	    //clean up any characters that turned out to be 91
		//we represent spaces(32) with 91 in our algorithm
		//to keep things consistent we sub then before encoding 
		//and back after decoding.
		int i = 0;
		while(isprint(cypher[i])){
		   if(cypher[i] == 32){ //32 is space
			  cypher[i] = 91; //91 because that is the next char after capitals
			  
		   }
		   if(key[i] == 32){
			  key[i] = 91;
		   }
		   int cypherChar = cypher[i] - 'A';
		   int keyChar = key[i] - 'A';
		//combine key char->num and cyph char->num
		   int decodeNum = cypherChar - keyChar;
		//mod the number
		   int decodeChar = modNum(decodeNum, 27) + 'A';
		//clean up any characters that turned out to be 91
		//we represent spaces(32) with 91 in our algorithm
		//to keep things consistent we sub then before encoding 
		//and back after decoding.
		   if(decodeChar == 91){
			  decodeChar = 32;
		   }
		   
		 //prepare the message for sending back.  
		   buffer[i]  = decodeChar;
		   i++;
		   //fprintf(stdout,"%c", decodeChar);
		   

		}
	   
	   ///printf("S: Encoded: %s\n", buffer);
	   
	   //send encoded data back
	   n = write(sock,buffer,strlen(buffer));
	   if (n < 0) error("ERROR reading from socket");
	   
	   bzero(key,25001);
	   bzero(cypher,25001);
   }
   while(!doneReading);
   
}
