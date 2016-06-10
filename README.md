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

# Assignment 4
*Summary:* An implementation of One Time Pad encryption. keygen generates a random key of specified length from 27 characters(A-Z and Space). Then otp_enc_d sever is launched in the background and creates a socket and listens on a provided port. otp_enc client then connects, gets pushed off to a new port in a new process by otp_enc_d to keep the server open for more connections. otp_enc then transmits the keyfile and plaintext to otp_enc_d to get encoded and gets back cyphertext. otp_dec_d and otp_dec are the reverse but function pretty much the same.

# Assignment 5
*Summary:* Writes files, generates random letters within a range, does math. The point was to see if we could pick up the basics of a new language without any instruction. The language being Python.