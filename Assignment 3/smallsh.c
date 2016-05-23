/*****************************************************
 * Author: Konstantin Yakovenko
 * Date: 2016/05/19
 * Assignment: #3 Small Shell
 * ***************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

//constants
#define MAX_INPUT_LENGTH 2048
#define MAX_INPUT_ARGS 512


//forward declarations
int inputLoop();
void exitSmallSh();
int cd(char *);
int status();

int main(){

   inputLoop();
   return 0;
}
//input loop. Cycle through inout and send it off to
//other functions.
int inputLoop(){
   //setup input
   printf(":");
   //read input
   char* input = NULL;
   ssize_t inputLength = 0;
   size_t inputSize = 0;
   char** args = malloc(inputLength *(sizeof(char*)));
   pid_t pid;

   char* output = NULL;
   const char* newLineChar = "\n";

   //
   while((inputLength = getline(&input, &inputSize, stdin)) > 0){

      char* readInput = strtok(input, newLineChar);

   //if status
      if(strcmp(readInput, "status") == 0){
         exitSmallSh();
      }
   //if cd
      if(strcmp(readInput, "cd") == 0){
         char cwd[1024];
         getcwd(cwd, sizeof(cwd));
         fprintf(stdout, "Current working dir: %s\n", cwd);

         //if only "cd" then goto $HOME
         //http://www.tutorialspoint.com/c_standard_library/c_function_getenv.htm

         readInput = strtok(NULL , newLineChar);
         if(readInput == NULL){
            char* homeAddr= getenv("HOME");
            if(chdir(homeAddr) == -1){
               perror("HOME Directory Change Failed");
               goto done;
            }
         }else{
            if(chdir(readInput) == -1){
               perror("Directory Change Failed");
               goto done;
            }
         }


      }
   //if exit
      if(strcmp(readInput, "exit") == 0){
         exitSmallSh();
      }
   //else search path/local

   //couldn't find?
   //
   done:
   printf(":");
   fflush(stdout);
   fflush(stdin);
   }
}

//changes the working directory to the path that is
//specified in the string passed along
int cd(char* destinationName){

}

//print either exit status or terminating signal
//of the lastforeground process.
int status(){

}

//must kill any other processes that the shell started
//before terminating itself
void exitSmallSh(){
   exit(0);
}