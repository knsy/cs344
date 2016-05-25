/*****************************************************
 * Author: Konstantin Yakovenko
 * Date: 2016/05/19
 * Assignment: #3 Small Shell
 * ***************************************************/

#include <stdio.h> //for perror(),
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h> //for booleans
#include <sys/wait.h> //for waiting
#include <sys/stat.h> //for chmod()
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h> //for STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO
#include <signal.h>


//constants
#define MAX_INPUT_LENGTH 2048
#define MAX_INPUT_ARGS 512
#define MAX_BG_PROCESS 10

const char* newLineChar = " \n";


//forward declarations
int inputLoop(int* bgProcess);
void exitSmallSh();
int cd(char*);
int status();

int processInput(char** args, unsigned int, char* inputBuffer, char** input, char** output, int* status, int* bgProcess);

int newFGProcess(char** args, char* input, char* output, int* status); //new foreground process
int newBGProcess(char** args, char* input, char* output, int* bgProcess); //new background process

//support functions
bool isComment(char*);
void trapInterrupt(); 



int main(){
   	 //background process array.
	int bgProcess[MAX_BG_PROCESS];

   	 //set all background PIDs to 0;
   	 int waitBGPindex = 0;
	 for(waitBGPindex = 0; waitBGPindex < MAX_BG_PROCESS; waitBGPindex++){
		bgProcess[waitBGPindex] = 0; 
	 }
	 
	//trap SIGINT. kill children.
   struct sigaction actInt;
   actInt.sa_handler = trapInterrupt;
   actInt.sa_flags = 0;
   sigfillset(&(actInt.sa_mask));
   
   sigaction(SIGINT, &actInt, NULL);
   
   fflush(stdout);
	fflush(stdin);
   
   inputLoop(bgProcess);
   return 0;
}
//input loop. Cycle through inout and send it off to
//other functions.
int inputLoop(int* bgProcess){
	int status = 0;	//this will be used to keep track of process status
	int bgpStatus = 0; //background process status
	pid_t bgPid = 0;	//temp pid storage for cycling through pids
	int waitBGPindex = 0;	 //temp index for cycling through pid array
	 
	//we pretty much don't come back here.
   //setup input
	fflush(stdout);
	fflush(stdin);
   printf(":");		//input line.

	//misc variables for taking in input
   char* inputBuffer = NULL;
   unsigned int inputLength;	
   size_t inputSize = 0;

	//read input
   //most of the action happens in here.
   while((inputLength = getline(&inputBuffer, &inputSize, stdin)) > 0){
	fflush(stdout);
   fflush(stdin);
   
   	 //wait for Background processes if any finished or got killed
	 for(waitBGPindex = 0; waitBGPindex < MAX_BG_PROCESS; waitBGPindex++){
		if(bgProcess[waitBGPindex] != 0){
			bgPid = waitpid(bgProcess[waitBGPindex], &bgpStatus, WNOHANG);
		}
		
		if(bgPid > 0){
		//shows status of the last command
		//http://www.chemie.fu-berlin.de/chemnet/use/info/libc/libc_23.html
		if (WIFEXITED(bgpStatus)){
				fprintf(stdout,"Background Process Exited:%i Exit Value: %i\n", bgPid, WEXITSTATUS(bgpStatus));
				bgProcess[waitBGPindex] = 0;
				}
		//status is a signal was sent
		if (WIFSIGNALED(bgpStatus)){
				fprintf(stdout, "Background Process Signaled:%i Exit Value: %i\n", bgPid, WSTOPSIG(bgpStatus));
				bgProcess[waitBGPindex] = 0;
				} 
		//if terminated
		if (WTERMSIG(bgpStatus)){
				fprintf(stdout, "Background Process Terminated:%i Terminated By: %i\n", bgPid, WTERMSIG(bgpStatus));
				}
		}
	 }
   
   //almost global variables for setting up where input and output for each command goes.
   char* output = NULL;
   char* input = NULL;
   char** args = malloc(MAX_INPUT_ARGS *(sizeof(char*)));
	printf(":");
     
     //here we process the lines that go in.
    processInput(args, inputLength, inputBuffer, &input , &output, &status, bgProcess);
    //printf("outOfInput input: %s output: %s\n", input, output);

    //printf("arrgs: %s", args[0]);

   //if status for foreground processes
      if(strcmp(args[0], "status") == 0){
		//shows status of the last command
		//http://www.chemie.fu-berlin.de/chemnet/use/info/libc/libc_23.html
		if (WIFEXITED(status)){
				fprintf(stdout,"Exit Value: %d\n", WEXITSTATUS(status));
				}
		//status is a signal was sent
		if (WIFSIGNALED(status)){
				fprintf(stdout, "Signal: %d\n", WSTOPSIG(status));
				}
		//status if terminated
		if (WTERMSIG(status)){
				fprintf(stdout, "Terminated: %d\n", WTERMSIG(status));
				}
		//restart the loop
        continue;
      }
      
   //if cd
      if(strcmp(args[0], "cd") == 0){
         //char cwd[1024];
         //getcwd(cwd, sizeof(cwd));
         //fprintf(stdout, "Current working dir: %s\n", cwd);

         //if only "cd" then goto $HOME
         //http://www.tutorialspoint.com/c_standard_library/c_function_getenv.htm
         //read the second argument and see if it is viable
         //if the next token is NULL, set the destination to $HOME
         if (args[1] == NULL){
            args[1] = getenv("HOME");
         }
         //cd either home or to the path provided.
         if(chdir(args[1]) == -1) {
            perror("Couldn't find this directory");
         }
         //getcwd(cwd, sizeof(cwd));
         //fprintf(stdout, "Current working dir: %s\n", cwd);
         continue; //restarts the loop
      }
      
   //if exit
      if(strcmp(args[0], "exit") == 0){
         exitSmallSh();
      }
   
   
   //since all the other ones didn't catch this, 
   //not a local function run in system in foeground
   if(!newFGProcess(args, input, output, &status)){
	   printf("Couldn't run this command...");
   }


   //flush, flush everywhere....
   //fflush(stdout);
   //fflush(stdin);
  
	}
   return 0;
}

int processInput(char** args, unsigned int length, char* inputBuffer, char** input, char** output, int* status, int* bgProcess){
	//grab a bite from the line
	char* readInput;
	readInput = strtok(inputBuffer, newLineChar); 
	//printf("RInput: %s", readInput);
	unsigned int args_index = 0;

	//keep processing while there is stuff to process
	while (readInput != NULL){
		
		//is our token a comment?
		if (isComment(readInput)) {
				//skip all that follows.
				break;
			
			} else if (strncmp(readInput, "<", 1) == 0) {
				//if there is an input symbol, grab the next token
				readInput = strtok(NULL, newLineChar);
				//if the next token is empty, we have no input
				if (readInput == NULL) {
					perror("No Input File Specified...\n");
					return args_index;
					}
				//otherwise assign input variable.
				*input = readInput;
				
				//prepare next input for next cycle of the loop
				readInput = strtok(NULL, newLineChar);
				//printf("we got input from: %s\n", *input);
				continue;
				
			} else if (strncmp(readInput, ">", 1) == 0) {
				//if there is an output character, grab next token
				readInput = strtok(NULL, newLineChar);
				//if the next token is empty, we have no input
				if (readInput == NULL) {
					perror("No Output FIle Specified...\n");
					return args_index;
				}
				//otherwise assign output variable.
				*output = readInput;
				
				//prepare next input for next cycle of the loop
				readInput = strtok(NULL, newLineChar);
				
				continue;
			} else if (strncmp(readInput, "&", 1) == 0) {
				//launch the process in background				
				if(!newBGProcess(args, *input, *output, bgProcess)){
				printf("Couldn't run this command...");
				}

				break;
			}
			
			//since it isn't < > & it is likely arguements
			//so we add them to the argument array
			args[args_index] = readInput;
			args_index++;
			readInput = strtok(NULL, newLineChar);
			} 
			//to launch a new process well need to have a
			//null index that specifies the end of our arguments array
			args[args_index] = NULL;
			
		
		//printf("endOfInput input: %s output: %s\n", *input, *output);
		return args_index;
}

//find if input is a comment. 
//helps trim comments from the ends of lines.
bool isComment(char *readInput) {
        int i;
        //parse through characters until the end of the input is reached
        for (i = 0; readInput[i] != '\0'; i++) {
			//if hash is found, then anything that follows is a comment
			if (readInput[i] == '#') {
				return true;
			}
        }
        return false;
}


/////////////////////////////////////////////newFGProcess//////////////V
int newFGProcess(char **args, char* input, char* output, int* status) {

	pid_t pid; //process id
	int inFile = STDIN_FILENO; //default in
	int outFile = STDOUT_FILENO; //default out
	//printf("FGP input: %s output: %s\n", input, output);

	//fork the process
	pid = fork();

	switch (pid) {
	//if fork returns 0, this means that this is the child process.
	case 0:
		//if there was input specified in the command...
		if (input != NULL) {
			//we try to open that input as READ ONLY as per instructions
			inFile = open(input, O_RDONLY);
			//if "open" fails...
			if (inFile == -1) {
				//we print error
				perror("Can't open input file");
				///I should run some cleanup here... atexit() perhaps...
				//and exit with a return of 1, as per instructions.
				exit(1);
				
			//if open doesn't fail...
			} else {
				//redirecting standard input into the infile
				//http://stackoverflow.com/questions/17518014/using-dup2-to-redirect-input-and-output
				dup2(inFile, STDIN_FILENO);
				//and now that we are done with inFile, we close it.
				close(inFile);
			}
		}
		
		//if there was output specified in the command...
		if (output != NULL) {
			//we try to open that output file as WRITE ONLY as per instructions
			//if it doesn't exist, we create it
			outFile = open(output, O_WRONLY | O_CREAT, 0644);
			//if opening and creating file fails...
			if (outFile == -1) {
				///I should run some cleanup here... atexit() perhaps...
				perror("Can't open or create ouput file");
				//and exit with a return of 1, as per instructions.
				exit(1);
			}
			
			//redirect the output to stdout...
			dup2(outFile, STDOUT_FILENO);
			//and close the outfile since we are done with it.
			close(outFile);
		}

		//now that input and output are setup, we can run the command
		if(execvp(args[0], args) == -1) {
			//if execution fails print error
			perror("Couldn't execute command");
			//and exit with a return of 1, as per instructions.
			exit(1);
			///I should run some cleanup here... atexit() perhaps...
			}
	
	//if fork returns -1, this means that we couldn't fork the process.
	case -1:
		perror("couldn't fork process");
		return 0;

	//if fork returns anything other than 0 or -1, this is parent.
	default:
		//since this is parent we will wait for the child process
		//to either finish naturally or be killed off by a signal.
		do {
			waitpid(pid, status, WUNTRACED);
		} while (!WIFEXITED(*status) && !WIFSIGNALED(*status));
	}

	return 1;
}



/////////////////////////////////////////////newBGProcess////////////////
int newBGProcess(char **args, char* input, char* output, int* bgProcess) {

	pid_t childPid = -6; //child's process id
	int inFile = STDIN_FILENO; //default in
	int outFile = STDOUT_FILENO; //default out
	//int inFile = open("/dev/null", O_RDWR); //default in
	//int outFile = open("/dev/null", O_RDWR); //default out
	//printf("FGP input: %s output: %s\n", input, output);

	//fork the process
	childPid = fork();

	switch (childPid) {
	//if fork returns 0, this means that this is the child process.
	case 0:
		//if there was input specified in the command...
		if (input != NULL) {
			//we try to open that input as READ ONLY as per instructions
			inFile = open(input, O_RDONLY);
			//if "open" fails...
			if (inFile == -1) {
				//we print error
				perror("Can't open input file");
				///I should run some cleanup here... atexit() perhaps...
				//and exit with a return of 1, as per instructions.
				exit(1);
				
			//if open doesn't fail...
			} else {
			
				//redirecting standard input into the infile
				//http://stackoverflow.com/questions/17518014/using-dup2-to-redirect-input-and-output
				dup2(inFile, STDIN_FILENO);
				//and now that we are done with inFile, we close it.
				close(inFile);
			}
		}
		
		//if there was output specified in the command...
		if (output != NULL) {
			//we try to open that output file as WRITE ONLY as per instructions
			//if it doesn't exist, we create it
			outFile = open(output, O_WRONLY | O_CREAT, 0644);
			//if opening and creating file fails...
			if (outFile == -1) {
				///I should run some cleanup here... atexit() perhaps...
				perror("Can't open or create ouput file");
				//and exit with a return of 1, as per instructions.
				exit(1);
			}else{
			
			
			//redirect the output to stdout...
			dup2(outFile, STDOUT_FILENO);
			
			//and close the outfile since we are done with it.
			close(outFile);
			}
		}

		//now that input and output are setup, we can run the command
		if(execvp(args[0], args) == -1) {
			//if execution fails print error
			perror("Couldn't execute command");
			//and exit with a return of 1, as per instructions.
			exit(1);
			///I should run some cleanup here... atexit() perhaps...
			}
			
	
	//if fork returns -1, this means that we couldn't fork the process.
	case -1:
		perror("Couldn't Fork Process");
		return 0;

	//if fork returns anything other than 0 or -1, this is parent.
	default:
		//as per instructions we print child pid:
		printf("Background pid is %i\n", childPid);
		
		
		int i;
		for (i = 0; i < MAX_BG_PROCESS; i++) {
			if (bgProcess[i] == 0) {
				bgProcess[i] = childPid;
			}
		//what if we faiL to save pid?
		
	
		}
	}

	return 1;
}



void trapInterrupt() {
/*	int bgpIndex = 0;
	for(bgpIndex = 0; bgpIndex < MAX_BG_PROCESS; bgpIndex++){
		if(bgProcess[bgpIndex] != 0){
			      kill(bgProcess[bgpIndex], SIGKILL);
		}
	}
	*/
}


//must kill any other processes that the shell started
//before terminating itself
void exitSmallSh(){
   exit(0);
}
