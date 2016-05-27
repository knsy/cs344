#!/usr/bin/python

import string
import random

#generate 10 random char string.
#http://stackoverflow.com/questions/2257441/random-string-generation-with-upper-case-letters-and-digits-in-python
def stringGen(size=10, chars=string.ascii_lowercase):
   return ''.join(random.choice(chars) for _ in range(size))

#we will loop to create new files
for i in range(1,4):
 
   #appending newline as per instructions
   s = stringGen() + '\n'
   print('File %d: %s' % (i,s))

   #creating a new string for file name cause I'm not sure
   #if I can do the name generation in the open() statement
   newFileName = 'File%d' % i

   #create a file
   #https://docs.python.org/2/tutorial/inputoutput.html
   newFile = open(newFileName, 'w')

  
   #write to file
   newFile.write(s)

   newFile.close()

#https://docs.python.org/2/library/random.html
int1 = random.randint(1,42)
int2 = random.randint(1,42)
print('Here are 2 random integers between 1 and 42, inclusive, and their product:\n' +
         'first:%d  second:%d  product:%d\n' % (int1, int2, int1 * int2))

