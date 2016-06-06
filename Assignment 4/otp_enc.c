/********************************************************
 * otp_enc is a client that attempts to connect to 
 * otp_enc_d on a specified port and sends over a KEY and 
 * PLAINTEXT that are then combined
 * into CYPHERTEXT and returned to the client.
 * USE: otp_enc PLAINTEXTFILE KEYFILE PORT
 * EXAMPLE: otp_enc myMessageFile myKeyFile 35555
 * WRITTEN BY: Konstantin Yakovenko
 * Based on the sample client/server from: 
 * http://www.linuxhowtos.org/C_C++/socket.htm
 * ******************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <ctype.h> //for iscntrl()


void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
	
	//somewhat arbitrary size. Large enough to satisfy our speed
    //requirements, doesn't go over int size.
    char buffer[25001];
    //check that the number of arguments is proper
    if (argc < 4) {
       fprintf(stderr,"usage %s plaintext key port\n", argv[0]);
       exit(0);
    }
    
    //get the PORT to connect to
    portno = atoi(argv[3]);
    
    //open and check files
    //open file to read
    int bytesRead = 0;
    FILE* inFile = fopen(argv[1], "r");
    if(inFile == NULL){
		perror("Can't open IN file");
		exit(1);
	}
	
	//check that there are no stupid unconvertable chars
	do{
		bytesRead = fread(buffer, 1, 25000, inFile);
		for(n = 0; n < (bytesRead - 1); n++){
			if(!isalpha(buffer[n]) && !isspace(buffer[n])){
			fprintf(stderr,"ERROR: %s contains unconvertable character\n", argv[1]);
			exit(1);
			}
		}
		
	}while(bytesRead > 0);
	
	//clear buffer to prevent any wacky errors.
	bzero(buffer,25001);

	
	//open key to read
	int bytesKey = 0;
    FILE* keyFile = fopen(argv[2], "r");
    if(keyFile == NULL){
		perror("Can't open KEY file");
		exit(1);
	}
		
	//measure that KEY is larger than PLAINTEXT
	fseek( inFile , 0L , SEEK_END);
	bytesRead = ftell( inFile );
	rewind( inFile );

	//measure that KEY is larger than PLAINTEXT
	fseek( keyFile , 0L , SEEK_END);
	bytesKey = ftell( keyFile );
	rewind( keyFile );
	
	//nope the f out if the key is not long enough
	if(bytesRead > bytesKey){
		perror("KEY is not long enough...");
		//close files
		fclose(inFile);
		fclose(keyFile);
		exit(1);
	}
	
	//reset to make sure no random bleedover happens
	bytesRead = 0;
	bytesKey = 0;
    
    
    
    
    //setup the first connection
    //this is the connection straight to the server
    //if the server likes us we'll take it to a different port
    //for some special one on one time...
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    
    //apparently we are doing this only on localhost
    //as per spec
    server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    //reset memory. maybe not really necessary but helps isolate problems
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno); //htons() converts to network bit order
    
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
        fprintf(stderr,"ERROR, can't connect to this port\n");
        exit(2); //as per spec.
	}
	///printf("C:connected....\n");
        
    //send 'enc' command to make sure that enc.client can't connect to dec.server
    n = write(sockfd,"enc",3);
    if (n < 0) 
         error("ERROR writing to socket");
    bzero(buffer,25001);
    n = read(sockfd,buffer,25000);
    if (n < 0) 
         error("ERROR reading from socket");
         
    //if rejected, quit.     
    if (buffer[0] == 'x'){
		perror("ERROR: otp_enc cannot use otp_dec_d");
		exit(1);
	}
         
    //here we have new port in buffer so we close old connection
    ///printf("New port:%s\n",buffer);
    close(sockfd);
    
    int splitPort = atoi(buffer);
    ///printf("New port int: %i\n", splitPort);
    
    //connect to the new port on the new process
    //resetupping most of the old stuff
    //THIS MIGHT BE UNNECESSARY, but clean slate and stuff.
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening new socket");
    server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such new host\n");
        exit(0);
    }
    
    //reset memory. maybe not really necessary but helps isolate problems
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(splitPort);//htons() converts to network bit order
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    
    /////////////////////////Send For Encryption//////////////////
  
	//keep track of CYPHER bytes we get back.
	int bytesCypher = 0;
	
	//read PLAINTEXT in sections
		bzero(buffer,25001);
		bytesRead = fread(buffer, 1, 25000, inFile);
		
    do{
		//to cut off the trailing newline;
		if(buffer[bytesRead - 1] == '\n'){
			buffer[bytesRead - 1] = '\0';
			bytesRead--;
		}
		
		//send the read PLAINTEXT line
		///printf("C:Read:%s\n",buffer);
		n = write(sockfd,buffer,strlen(buffer));
		if (n < 0) 
         error("ERROR writing to socket");
        
         //read KEY in sections
		bzero(buffer,25001);
		bytesKey = fread(buffer, 1, bytesRead, keyFile);
		
		//on shorter tasks server needs to catchup.
		//sleep(1); too much
		//usleep(100), not enough
		usleep(10000);//is in MICRO sec so x1000 to get MILLI sec
       
        //send the KEY line
		///printf("C:Key:%s\n",buffer);
		n = write(sockfd,buffer,strlen(buffer));
		if (n < 0) 
         error("ERROR writing to socket");
        
        //read encoded feedback aka CYPHER
        bzero(buffer,25001);
		n = read(sockfd,buffer,25000);
		if (n < 0) 
         error("ERROR reading from socket");
	
		///printf("C:Encoded:%s\n",buffer);
		//write CYPHER out
		///bytesCypher = fprintf(cypherFile,"%s",buffer);
		bytesCypher = fprintf(stdout,"%s",buffer);
		
		//get next part if there is one
		bzero(buffer,25001);
		bytesRead = fread(buffer, 1, 25000, inFile);
	}
	while(bytesRead > 0);
	
	//finish close off the file or stream
	 bytesCypher = fprintf(stdout,"\n");   
	
	//send that we are done sending shit
	n = write(sockfd,"done",4);
	if (n < 0) 
	 error("ERROR writing to socket");

    //close files
    fclose(inFile);
    fclose(keyFile);
    //close connection
    close(sockfd);
    
    return 0;
}
