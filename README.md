# cs344
Intro to Operating Systems. Bash scripting, C, File I/O, Processes.

# Assignment 1
*Summary:* A BASH script that calculates row or column statistics on data passed into it either by a file or a stream or manually.

# Assignment 2
*Summary:* Build a game text-based game in C that generates rooms and randomly connects them 
designating one as a start room and one as an end room. You then proceed to go from room to room until you find the end room and win. 
This project also deals with file I/O. Rooms are written to the disk as separate files and then read back into the program and parsed.

# Assignment 3
*Summary:* A linux shell that interprets commands. Integrated commands include exit, cd(to change the working directory), and status(to show the exit status of the last command). Other commands get interpreted according to the following syntax:
```
command [arg1 arg2 ...] [< input_file] [> output_file] [&]
```
IO gets redirected if need be, then a new process gets exec'd with the parameters passed. If the "&" is included the process gets pushed to the background.
