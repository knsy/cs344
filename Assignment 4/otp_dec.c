/*************************************************************
 * Connects to otp_enc_d and sends a keyfile and a plaintext file
 * receives cypher.
 * USAGE:otp_dec plaintext key port
 * BY: Konstantin Yakovenko
 * **********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 


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
	
    char buffer[256];
    if (argc < 4) {
       fprintf(stderr,"usage %s plaintext key port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[3]);
    
    //open and check files
     //open file to read
    int bytesRead = 0;
    FILE* inFile = fopen(argv[1], "r");
    if(inFile == NULL){
		perror("Can't open IN file");
		exit(1);
	}
	
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
	
	if(bytesRead > bytesKey){
		perror("KEY is not long enough...");
		//close files
		fclose(inFile);
		fclose(keyFile);
		exit(1);
	}
	
	bytesRead = 0;
	bytesKey = 0;
    
    
    
    
    //setup the first connection
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    
    //server = gethostbyname(argv[1]);
    server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
	///printf("C:connected....\n");
        
    //send 'enc' command to make sure that enc.client cand connect to dec.server
    //fgets(buffer,255,stdin);
    n = write(sockfd,"dec",3);
    if (n < 0) 
         error("ERROR writing to socket");
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0) 
         error("ERROR reading from socket");
    
    //if rejected quit.     
    if (buffer[0] == 'x'){
		perror("C: can't connect to otp_enc_d");
		exit(1);
	}
         
    //here we have new port in buffer so we close old connection
    ///printf("New port:%s\n",buffer);
    close(sockfd);
    
    int splitPort = atoi(buffer);
    ///printf("New port int: %i\n", splitPort);
    
    //connect to the new port on the new process
    //THIS MIGHT BE UNNECESSARY
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening new socket");
    server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such new host\n");
        exit(0);
    }
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(splitPort);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    /*printf("Please enter the second message: ");
    bzero(buffer,256);
    fgets(buffer,255,stdin);
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0) 
         error("ERROR writing to socket");
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0) 
         error("ERROR reading from socket");
    
    //return msg
    printf("Cli Received:%s\n",buffer); */
    
    /////////////////////////Send For Encryption//////////////////
    
    
   
	
	//open CYPHER to write
	//ERROR Do we need to clear/initiate this file prior
	//to jumping into the loop?
	int bytesCypher = 0;
    ///FILE* cypherFile = fopen("cypherfile", "a");
    ///if(cypherFile == NULL){
		///perror("Can't open CYPHER file");
		///exit(1);
	///}
	
	//read PLAINTEXT in sections
		bzero(buffer,256);
		bytesRead = fread(buffer, 1, 255, inFile);
		
    do{
		//to cut off the trailing newline;
		if(buffer[bytesRead - 1] == '\n'){
			buffer[bytesRead - 1] = '\0';
			bytesRead--;
		}
		
		//send the read line
		///printf("C:Read:%s\n",buffer);
		n = write(sockfd,buffer,strlen(buffer));
		if (n < 0) 
         error("ERROR writing to socket");
         
         
         
         //read KEY in sections
		bzero(buffer,256);
		bytesKey = fread(buffer, 1, bytesRead, keyFile);
		
		//on shorter tasks server needs to catchup.
		//sleep(1);
		//100 not enough
		usleep(300);//is in MICRO sec so x1000 to get MILLI sec
		
         //send the key line
		///printf("C:Key:%s\n",buffer);
		n = write(sockfd,buffer,strlen(buffer));
		if (n < 0) 
         error("ERROR writing to socket");
        
        //read encoded feedback
        bzero(buffer,256);
		n = read(sockfd,buffer,255);
		if (n < 0) 
         error("ERROR reading from socket");
	
		///printf("C:Encoded:%s\n",buffer);
		//write CYPHER to file of OUT
		///bytesCypher = fprintf(cypherFile,"%s",buffer);
		bytesCypher = fprintf(stdout,"%s",buffer);
		
		//get next part
		bzero(buffer,256);
		bytesRead = fread(buffer, 1, 255, inFile);
	}
	while(bytesRead > 0);
	
	 bytesCypher = fprintf(stdout,"\n");   
	
	//printf("done.\n");
	//send that we are done
	n = write(sockfd,"done",4);
	if (n < 0) 
	 error("ERROR writing to socket");

    //close files
    fclose(inFile);
    fclose(keyFile);
    ///fclose(cypherFile);
    //close connection
    close(sockfd);
    
    return 0;
}
