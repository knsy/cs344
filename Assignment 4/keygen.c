/******************************************************
 * keygen.c generates random keys for use with OTP encode 
 * and decode.
 * USE: keygen keyLength 
 * EXAMPLE: keygen 1024 > keyFile
 * WRITTEN BY: Konstantin Yakovenko
 * ***************************************************/

#include <stdlib.h> //for random()
#include <stdio.h> //for input/output

int main(int argc, char* argv[]){
   if (argc < 2){
      perror("Error, not enough arguements. Correct Usage: keygen keyLength");
      exit (1);
   }

   int numLetters = atoi(argv[1]);
   char newRandLetter = 'A';
   int lettersGenerated = 0;
   for(lettersGenerated = 0; lettersGenerated < numLetters; lettersGenerated++){
      newRandLetter = 'A' + (random() % 27);

      //since we need to include a space, we sub ASCII 91 for a space
      if(newRandLetter == 91){
         newRandLetter = 32;
      }
      fprintf(stdout, "%c", newRandLetter);
   }
   
   //newfile to close off the file
   fprintf(stdout, "\n");

return 0;

}
