#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#define TOTAL_ROOMS 10
#define SELECTED_ROOMS 7
#define MAX_CONNECTIONS 6
#define MIN_CONNECTIONS 3
//create array for room names
const char* allRoomNames[] = {"room6","dungeon","castle",
                                          "forest","field","tower",
                                          "desert","church","swamp","mountain"};


struct roomStruct{
   char roomName[255];
   struct roomStruct* roomConnections[6]; //for keeping track of connections
   int roomType; //1 = START_ROOM, 2 = MID_ROOM, 3 = END_ROOM
   int numberOfConnections;
};

struct roomStruct* allRooms[SELECTED_ROOMS];
 
//forward declarations
char* workDirName();
int exportRoom(struct roomStruct* roomToExport);

int createRoomList();
int buildRoomConnections();
main(){

//http://stackoverflow.com/questions/7430248/creating-a-new-directory-in-c
int created = mkdir(workDirName(), 0777);
//changing working directory because we will be writing and reading a lot
//http://stackoverflow.com/questions/14179559/changing-working-directory-in-c
int dirChanged = chdir(workDirName());

//seed random generator with current time.
//http://stackoverflow.com/questions/822323/how-to-generate-a-random-number-in-c
srand((unsigned int)time(NULL));

createRoomList();
buildRoomConnections();
int count;
for(count = 0; count < SELECTED_ROOMS; count++){
exportRoom(allRooms[count]);
}

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
         }while(pickedRooms[randomRoomName] == 1 && randomRoomName == i);
         
         //make the connection if the room on the other end
         //isn't full on its' connections
         if(allRooms[randomRoomName]->numberOfConnections < MAX_CONNECTIONS){
         //connect rooms in one direction
         allRooms[randomRoomName]->roomConnections[allRooms[randomRoomName]->numberOfConnections] = allRooms[i];
         //mark that we connected
         allRooms[randomRoomName]->numberOfConnections++;

         //connect rooms in the other direction
         allRooms[i]->roomConnections[allRooms[i]->numberOfConnections] = allRooms[randomRoomName];
         //mark that we connected
         allRooms[i]->numberOfConnections++;

         }         

         //mark the room we just looked at so that we don't touch it again.
         pickedRooms[randomRoomName] = 1;
      }


   }
return 0;
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
