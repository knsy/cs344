//This is a game that creates a list of rooms, randomly connects them
//and then lets you try to go through rooms until you 
//find the end room that the game has initially designated as such.
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


#define TOTAL_ROOMS 10
#define SELECTED_ROOMS 7
#define MAX_CONNECTIONS 6
#define MIN_CONNECTIONS 3


//create array for room names
//these are to be hardcoded
const char* allRoomNames[] = {"room6","dungeon","castle",
                              "forest","field","tower",
                              "desert","church","swamp","mountain"};

//this struct will serve as a basis for a room
struct roomStruct{
   char roomName[255];  //name of the room
   struct roomStruct* roomConnections[6]; //for keeping track of connections
   int numberOfConnections;   //for keeping track of # of connections
   int roomType; //1 = START_ROOM, 2 = MID_ROOM, 3 = END_ROOM
};

//these will be our collections of rooms for easy reference
//this one is used first to generate rooms and then save them as files
struct roomStruct* allRooms[SELECTED_ROOMS];
//this one is used gets repopulated from files for file parsing practice
struct roomStruct* allReadRooms[SELECTED_ROOMS];
 

//forward declarations
int envSetup();
int readFiles();
char* workDirName();
int exportRoom(struct roomStruct* roomToExport);
int alreadyConnected(struct roomStruct* firstRoom, struct roomStruct* secondRoom);
int createRoomList();
int buildRoomConnections();
int printRoom(struct roomStruct* roomToPrint);
int printGameRoom(struct roomStruct* roomToPrint);
int playGame();

main(){
   //this will setup our working environment. Directory/relocation/random seed
   envSetup();

   //now we will randomly pick rooms from that list and start filling
   //our room structures
   createRoomList();

   //now that we know where what room is, we randomly connect them
   //number of connections is random btwn 3 and 6
   //all rooms are doubly linked to ensure a traversal to the end exists
   buildRoomConnections();

   //now we write all the rooms into files for later reading
   int count;
   for(count = 0; count < SELECTED_ROOMS; count++){
      exportRoom(allRooms[count]);

      //this outputs all the rooms for cheating....errr....debugging.
      //printRoom(allRooms[count]);
   }

   //to practice file parsing we read the saved files back into a new
   //structure. The indeces of the rooms are not necessarily the same
   //but the starting room is the same and that is what matters.
   readFiles();

   //FOR TESTING
   //prints all the rooms we just read in.
   //for(count = 0; count < SELECTED_ROOMS; count++){
   //   printRoom(allReadRooms[count]);
   //}

   //now that all the structures are setup
   //this is the game environment
   playGame();

   return 0; 
}


//Environment setup. 
int envSetup(){
    //create a working directory in the format "yakovenk.rooms.<pid>"
    //http://stackoverflow.com/questions/7430248/creating-a-new-directory-in-c
    int created = mkdir(workDirName(), 0777);

    //changing working directory because we will be writing and reading a lot
    //http://stackoverflow.com/questions/14179559/changing-working-directory-in-c
    int dirChanged = chdir(workDirName());

    //seed random generator with current time.
    //http://stackoverflow.com/questions/822323/how-to-generate-a-random-number-in-c
    srand((unsigned int)time(NULL));
}

//Reads files into the allReadRooms[]
int readFiles(){

   //read in file line by line
   //http://stackoverflow.com/questions/3501338/c-read-file-line-by-line
   FILE* fileRead;
   char roomName[20]; //should be enough for our rooms

   int newRoomIndex = 0;
   //scroll through the names of the rooms we have
   //and see which ones we stumble on.
   //the ones we find, we'll just create named empty structs for
   int i;
   for(i = 0; i < TOTAL_ROOMS; i++){
      //check if the file exist
      //http://stackoverflow.com/questions/13945341/c-check-if-a-file-exists-without-being-able-to-read-write-possible
      struct stat st;
      int roomExists = stat(allRoomNames[i], &st);
      if (roomExists == 0){
         fileRead = fopen(allRoomNames[i], "r");

         //make sure the file exists
         if(fileRead == NULL){
            return 1;
         }

         //allocate space for new room struct
         struct roomStruct* tempRoom = malloc(sizeof(struct roomStruct));
         tempRoom->numberOfConnections = 0;
         //read line
         const size_t lineSize = 100;
         char* readLine = malloc(lineSize);

         //make sure the file read pointer is in the beginning
         rewind(fileRead);
         fscanf(fileRead, "ROOM NAME: %s\b", roomName);
         sprintf(tempRoom->roomName,roomName);
 
         //attach the new room into the array
         allReadRooms[newRoomIndex] = tempRoom;
         newRoomIndex++; //there should be only 7 rooms in that directory, so I 
                        // am hoping that there will only be 7.

         //free up the line ptr
         free(readLine);
         //close file
         fclose(fileRead);
      }
  }


   //now that the array with names is created we can process connections
   int numConn;
   for(i = 0; i < SELECTED_ROOMS; i++){
      //we pick a name from our array
      fileRead = fopen(allReadRooms[i]->roomName, "r");
      
      //get past the first line with the name which we already processed
      char dump[50];
      fgets(dump, 50, fileRead);
      int successfulRead;
         //this was really helpful with fscanf formatting
         //http://stackoverflow.com/questions/3373556/using-fscanf-to-read-in-a-integer-from-a-file
         while((successfulRead = fscanf(fileRead, "%*s%i: %s\b", &numConn, roomName))
             != 0 ){ 

            //find index of the room we are linking to
            int linkIndex = findIndex(allReadRooms, roomName);

            //create a link. Unlike when we were creating initial links,
            //here we will NOT be double linking, that should be covered
            //if the files get read correctly.
            if( linkIndex >= 0){
               allReadRooms[i]->roomConnections[allReadRooms[i]->numberOfConnections] 
                  = allReadRooms[linkIndex];
               
               //making sure to keep track of the number of links.
               allReadRooms[i]->numberOfConnections++;
            }
         }
         //so at this point our file reading pointer is all at the very end....
         //we know that the last line is actually pretty consistent in length...
         //soooo.... we'll try scrolling back just a tad...about 22? 25? chars.
         //feels sooo janky.
         int back = -25;
         char roomType[20];
         fseek(fileRead, back, SEEK_END);
         
         //now we discart the ending of this line and...
         fscanf(fileRead, "%s", roomType);

         //this allows us to use all the formatting glory of fscanf on the last line
         fscanf(fileRead, "%*s%*s%*c%s", roomType);
        
         //luckily we can classify our room types byt just the first char
         //START_ MID_ END_
         if(roomType[0] == 'S'){
            allReadRooms[i]->roomType = 0;
         } 
         if(roomType[0] == 'M'){
            allReadRooms[i]->roomType = 1;
         } 
         if(roomType[0] == 'E'){
            allReadRooms[i]->roomType = 2;
         } 

   }

   //now that the connections are back up, we can sort them based on the type
   //it is important that allReadRooms[0] is the starting room because that
   //is where the game starts...other ones are actually not very important.

   //find the starting room index
   for(i = 0; i < SELECTED_ROOMS; i++){
      if(allReadRooms[i]->roomType == 0){
         //switch the starting room to the 0'th index.
         struct roomStruct* tempRoomPtr = allReadRooms[0];
         allReadRooms[0] = allReadRooms[i];
         allReadRooms[i] = tempRoomPtr;
      }
   }
}

//find index of the room by name
//returns index, if not found returns -1.
int findIndex(struct roomStruct* roomArray[], char* roomName){
   int i;
   for(i = 0; i < SELECTED_ROOMS; i++){
      int compareNames = strcmp(roomArray[i]->roomName,roomName); 
      if( compareNames == 0){
         return i;
      }
   }
   return -1;
}

//game loop
int playGame(){
   //all of this will be based around this "current room" which starts
   //at the starting room, index 0 in our room collection
   struct roomStruct* currentRoom = allReadRooms[0];
   int stepsTaken = 0; //keeping track of the steps we take.
   
   //not sure of the best way to keep the path history...
   //array of indeces? everything I thought of had similar problems
   //soo... I just decided to make a big string and dump the names in 
   //there. I risk that this array will run out if the user twats around too much
   //but with only 7 rooms... this should be enough.
   char pathTaken[500]; 
      
   //we will cycle through these rooms until we reach a room of type 2, aka END_ROOM
   while(currentRoom->roomType != 2){
      printGameRoom(currentRoom);

      // take instruction where to go...
      // https://en.wikibooks.org/wiki/C_Programming/Simple_input_and_output
      char userInput[50]; //size should be based on the longest name of used rooms but...
   
      fgets(userInput, 50, stdin);

      //input contains a newline char at the end that screws with our comparisons
      //http://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
      strtok(userInput,"\n");
      int roomFound = 0;   //have we found the room?
      int foundRoomIndex = 0; //at what index?
      int i;   //just a counter.
      for(i = 0; i < currentRoom->numberOfConnections; i++){

         //see if any of our names match as we cycle through them.
         int namesEqual = strcmp(userInput, currentRoom->roomConnections[i]->roomName);
         if(namesEqual == 0){
            roomFound = 1;
            foundRoomIndex = i;
         }
      }

      if(!roomFound){
         //soo... we couldn't find what you typed in.... try again.
         printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n");
      }else{
         //we found the room you are lookig for! Lets move there...and start all over.
         currentRoom = currentRoom->roomConnections[foundRoomIndex];
         stepsTaken++;  //keeping track of the steps. This is like a pedometer.

         //record the path taken;
         //http://www.tutorialspoint.com/c_standard_library/c_function_strncat.htm
         strncat(pathTaken, currentRoom->roomName, 20); 
         strncat(pathTaken, "\n", 4); 
      }
   } 
   //if you got here, I guess you found the END_ROOM
   printf("\nYOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
   printf("YOU TOOK %i STEPS. YOUR PATH TO VICTORY WAS:\n%s", stepsTaken, pathTaken);
}

//print game room prints instructions for current room in the game
int printGameRoom(struct roomStruct* roomToPrint){
   
   printf("\nCURRENT LOCATION: %s\n", roomToPrint->roomName);
   printf("POSSIBLE CONNECTIONS:");

   int i; //loop counter for cycling through connections

   //go through the room connections and print them out
   //for the person to pick from
   for(i = 0; i < roomToPrint->numberOfConnections; i++){
      printf(" %s", roomToPrint->roomConnections[i]->roomName);
      if( i < (roomToPrint->numberOfConnections - 1)){
         printf(",");
      } else {
         printf(".\n");
      }
   }

   printf("WHERE TO?>");
   return 0;
}

//generate a room and populate the room list
int createRoomList(){
   //populate our array of rooms based on randomly selected names
   //no repeating names.
   int pickedRooms[TOTAL_ROOMS] = {0,0,0,0,0,0,0,0,0,0};
   int randomRoomName;
   int i;
   for(i = 0; i < SELECTED_ROOMS; i++){
      //pick a random number for room name
      do{
         randomRoomName = rand() % TOTAL_ROOMS;
      }while(pickedRooms[randomRoomName] == 1);
      //make sure the number isn't repeating.

      //create a new struct and fill it with a new name
      struct roomStruct* tempRoom = malloc(sizeof(struct roomStruct));
      sprintf(tempRoom->roomName, allRoomNames[randomRoomName]);
      
      //insert the new struct into the array of all rooms
      allRooms[i] = tempRoom;
   
      //mark the name used
      pickedRooms[randomRoomName] = 1;

      //initialize number of connections
      allRooms[i]->numberOfConnections = 0;
   }

   //at this point we have an array of rooms with names.
   //first in this array is the START_ROOM; which we denote with a 0.
   allRooms[0]->roomType = 0;

   //last in the array is the END_ROOM; which we denote with a 2.
   allRooms[(SELECTED_ROOMS - 1)]->roomType = 2;
   
   //inbetween those are MID_ROOM's; which we denote with a 1;
   for(i = 1; i < (SELECTED_ROOMS - 1); i++){
   allRooms[i]->roomType = 1;
   }
   return 0;
}

//here we will link rooms. Each room will have between 3 and 6 connections
int buildRoomConnections(){
   int connRange = MAX_CONNECTIONS - MIN_CONNECTIONS;
   int i;
   //now we cycle through rooms and use i to refer to the one we are working on
   for(i = 0; i < SELECTED_ROOMS; i++){
      //random number of connections for each room
      int connToCreate = (rand() % connRange) + MIN_CONNECTIONS;
      
      //while we haven't reached the number of desired connections...     
      while(allRooms[i]->numberOfConnections < connToCreate){
         //populate our array of rooms based on randomly selected names
         //no repeating names.
         int pickedRooms[SELECTED_ROOMS] = {0,0,0,0,0,0,0};
         int randomRoomName;
         //pick a room to connect to
         //pick a random number for room name that hasn't already 
         //been connected to and that isn't the room itself
         do{
            randomRoomName = rand() % SELECTED_ROOMS;
         }while(pickedRooms[randomRoomName] == 1 || randomRoomName == i);
 
         //mark the room we just looked at so that we don't touch it again.
         pickedRooms[randomRoomName] = 1;
         
         //make the connection if the room on the other end
         //isn't full on its' connections
         if(allRooms[randomRoomName]->numberOfConnections < MAX_CONNECTIONS){
            //check also that the room doesn't already have the backwards connection
            if(!alreadyConnected(allRooms[i],allRooms[randomRoomName])){
            //connect rooms in one direction
            allRooms[randomRoomName]->roomConnections[allRooms[randomRoomName]->numberOfConnections] = allRooms[i];
            //mark that we connected
            allRooms[randomRoomName]->numberOfConnections++;

            //connect rooms in the other direction
            allRooms[i]->roomConnections[allRooms[i]->numberOfConnections] = allRooms[randomRoomName];
            //mark that we connected
            allRooms[i]->numberOfConnections++;
            }
         }         
     }


   }
return 0;
}

//check if the two rooms are already connected
int alreadyConnected(struct roomStruct* firstRoom, struct roomStruct* secondRoom){
   int connected = 0;
   int j; //connections from the secondRoom to the First.
      for(j = 0; j < secondRoom->numberOfConnections; j++){
         int compareNames = strcmp(secondRoom->roomConnections[j]->roomName, 
                                    firstRoom->roomName);
         if(compareNames == 0){
            connected = 1;
         }
   }

   return connected;
}

int exportRoom(struct roomStruct* roomToExport){
//exports a room into a file
//http://www.tutorialspoint.com/cprogramming/c_file_io.htm
FILE* filePtr;


filePtr = fopen(roomToExport->roomName,"w+");
//print room name;
fprintf(filePtr, "ROOM NAME: %s\n", roomToExport->roomName);

//PRINT CONNECTIONS HERE.
int i;
for(i = 0; i < roomToExport->numberOfConnections; i++){

fprintf(filePtr, "CONNECTION %i: %s\n", i, roomToExport->roomConnections[i]->roomName);

}

//print room type;
if(roomToExport->roomType == 0){
fprintf(filePtr, "ROOM TYPE: START_ROOM\n" );
}
if(roomToExport->roomType == 1){
fprintf(filePtr, "ROOM TYPE: MID_ROOM\n" );
}
if(roomToExport->roomType == 2){
fprintf(filePtr, "ROOM TYPE: END_ROOM\n" );
}
fclose(filePtr);

return 0;
}

//create a char array that will contain the path to the temporary
//directory. Temp directory is named yakovenk.rooms.<pid>
char* workDirName(){
   //get the pid
   //http://ubuntuforums.org/showthread.php?t=1430052
   int pid = getpid();

   //allocate space for the string
   //should probably calculate it but 255 seems like enough for now
   char *workDir = malloc(255 * sizeof(char));
   assert(workDir != NULL);

   //composite the name and pid into one string
   sprintf(workDir, "yakovenk.rooms.%i", pid);

   return workDir;
}

//FOR TESTING-------------------------------------------------
//print out a room on the screen
//used for testin purpouses currently but is likely to be adapted to 
//the game interface when I get to that.
int printRoom(struct roomStruct* roomToPrint){

//print room name;
printf("ROOM NAME: %s\n", roomToPrint->roomName);

//PRINT CONNECTIONS HERE.
int i;
for(i = 0; i < roomToPrint->numberOfConnections; i++){

printf("CONNECTION %i: %s\n", i, roomToPrint->roomConnections[i]->roomName);
}

//print room type;
if(roomToPrint->roomType == 0){
printf("ROOM TYPE: START_ROOM\n" );
}
if(roomToPrint->roomType == 1){
printf("ROOM TYPE: MID_ROOM\n" );
}
if(roomToPrint->roomType == 2){
printf("ROOM TYPE: END_ROOM\n" );
}

printf("--------------------------\n\n");
return 0;
}

