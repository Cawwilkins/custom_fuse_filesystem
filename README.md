## INFO
This was a project for one of my classes that I chose to include in my portfolio. The below README is what I turned in. 


## Project Description
This is a custom single-level filesystem using FUSE to implement it all from userspace. The user has basic fileystem functionality
such as creating and deleting files, writing and appeneding to them, and displaying contents. This project does not allow for the
creation of additional folders. 


## How to Compile Your Project 
1. Run 'make build_img'
2. Run 'make driver'
3. Run './simplefs-main simplefs.img -f mountfolder' - I used testpoint as my foldername 
4. Open a new terminal and navigate to mounted folder
5. When done, unmount by running 'fusermount -u mountfolder'


## Sample Run of the Project, Demonstrate how to use it.
After running './simplefs_main simplefs.img -f testpoint' with testpoint being your mounting point,
open a separate terminal and enter the mountpoint folder. Here is a run of several commands and their
output that validates they work:

vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins/testpoint$ ls
vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins/testpoint$ touch abc
vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins/testpoint$ ls
abc
vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins/testpoint$ echo "hi" >> abc
vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins/testpoint$ cat abc
hi
vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins/testpoint$ echo "world" >> abc
vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins/testpoint$ cat abc
hi
world
vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins/testpoint$ touch xyz
vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins/testpoint$ ls
abc  xyz
vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins/testpoint$ rm abc
vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins/testpoint$ ls
xyz
vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins/testpoint$ ls -l
total 0
-rw-r--r-- 1 vboxuser vboxuser 0 Dec  5 19:46 xyz
vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins/testpoint$ echo "Hello world" >> xyz
vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins/testpoint$ ls -l
total 0
-rw-r--r-- 1 vboxuser vboxuser 12 Dec  5 19:47 xyz
vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins/testpoint$ cat xyz
Hello world
vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins/testpoint$ rm xyz
vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins/testpoint$ ls
vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins/testpoint$ ls -l
total 0
vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins/testpoint$ cd ..
vboxuser@CMSC-421:/usr/src/project-5/project5-fa25-Cawwilkins$ fusermount -u testpoint


## Known Project Issues
1. None known


## LLM/AI Prompts Used
1. Is this in network byte order and if so what is this in normal order: I was trying to determine if i was getting garbage data or if my stuff wasn't properly 
switched betwen ntohs and htons.
2. Here is a screenshot of an error I am getting, what does this even mean: When I first started with fuse I was completely confused by the file permission errors
3. Do you have any resourses for the fuse functions: I needed the function signatures for the fuse functions
4. For some reason my code wont even open the image file I am referencing in my command line statement, is just indexing by each statement enough to actually find a file?: I put a 
stub in my init and found out it wasnt even opening the image file. I was trying to determine if I was doing it the right way or if I needed to use the absolute path like I ended up needing to?
5. If I cast as a type will it stay that way or is it just local to the one line: I wanted to cast the msb and bsb signatures so I could use strcmp
6. Can you explain how lseek works in a linux and C environment?: I had seen online people used lseek with write() so I was trying to determine how to use it and if I needed it
7. When do I need to null terminate?: String parsing errors, xoring errors, etc. which made me think of needing to potentially null terminate.


## Sources Used
1. https://github.com/libfuse/libfuse/blob/master/example/hello.c
2. https://libfuse.github.io/doxygen/fuse_8h_source.html
3. https://libfuse.github.io/doxygen/structfuse__operations.html#ac39a0b7125a0e5001eb5ff42e05faa5d
4. https://man7.org/linux/man-pages/man3/stat.3type.html
5. https://www.geeksforgeeks.org/software-engineering/bitwise-xor-operator-in-programming/
6. https://libfuse.github.io/doxygen/fuse_8h.html
7. https://www.w3schools.com/c/c_date_time.php
8. https://stackoverflow.com/questions/18079449/is-there-a-system-call-for-obtaining-the-uid-gid-of-a-running-process
9. https://kohminds.com/explore/featured_Linux-File-Permissions-explained-A-Quick-and-Practical-Guide
10. https://github.com/libfuse/libfuse/blob/master/example/hello_ll.c
11. https://man7.org/linux/man-pages/man3/errno.3.html
12. https://stackoverflow.com/questions/2341808/how-to-get-the-absolute-path-for-a-given-relative-path-programmatically-in-linux
13. https://geeksforgeeks.org/cpp/strdup-strdndup-functions-c/
14. https://man7.org/linux/man-pages/man2/lseek.2.html
15. https://stackoverflow.com/questions/28633343/find-index-of-pointer-array-in-c
