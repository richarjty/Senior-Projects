/*-----------------------------------------------------------------------------------------
Programming Assignment 3

Simulate File System Partition 

Written By   :    Jon Richardson
Version      :    November 5, 2020

-----------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "pa3.h"
//#include "util.c"
//#include "dir.c"
//#include "file.c"



bool mounted;
char *partition;
extern struct tableEntry fileTable[32];

    char *input;
    size_t buffersize = 100;
    size_t characters;
    char *buffer;
    int fd;
    int read;
    int bytes;
    int seek;
    FILE *stu;
    char *n;

int main( int argc , char *argv[] ) 
{
    //bool v = false;           // verbose flag - if true, print out detailed process information

    bool end = false;
    mounted = false;
    FILE *f;
    //char *filename; // = "partition.bin";


    input = (char *)malloc(buffersize * sizeof(char));
    n = (char *)malloc(buffersize * sizeof(char));
    partition = (char *)malloc(buffersize * sizeof(char));
    

    if( input == NULL)
    {
        perror("Unable to allocate input buffer\n");
        exit(-1);
    }

    while(!end) {


        menu(&end);

    }


    free(input);
    free(partition);
    free(n);
    printf("Program ending\n");

}

void menu (bool *end) {

    int choice = 0;
    printf("---------------------------------------------------------------------- \n");
    printf("Mounted partition is: %s \n", partition); 
    printf("---------------------------------------------------------------------- \n");
    printf("Enter the number of the desired option \n");
    printf("1)  Format a file system \n");
    printf("2)  Mount a file system \n");
    printf("3)  Create a directory \n");
    printf("4)  Remove a directory \n");
    printf("5)  List the contents of a directory \n");
    printf("6)  Create a file \n");
    printf("7)  Remove a file \n");
    printf("8)  Open a file \n");
    printf("9)  Read from a file \n");
    printf("10) Write to a file \n");
    printf("11) Seek to a location in a file \n");
    printf("12) Close a file \n");
    //printf("13) Recover a deleted file \n");
    printf("13) Exit the program \n");

    choice = getNum();

    switch(choice) {


        case(1) :

             printf("Name of file to format? \n");            
             getString();

            if(0 == format(input)){
                printf("%s successfully formatted.\n", input);
            } else {
                printf("%s Failed to format.\n", input);
            }

            break;
        case(2) :

             printf("Name of file to mount? \n");            
             getString();

            if(0 == mount(input)){
                printf("%s successfully mounted.\n", input);
                strcpy(partition, input);
                mounted = true;
            } else {
                printf("%s Failed to mount.\n", input);
            }

            break;
        case(3) :

             printf("Path of directory to make? \n");            
             getString();

             //printf("Partition is this: %s \n", partition);  
             //printf("Input is this: %s \n", input);

            if(0 == Dir_Create(input)){
                printf("%s successfully created.\n", input);
            } else {
                printf("%s creation failed.\n", input);
            }

            break;
        case(4) :
             printf("Path of directory to remove? \n");            
             getString();

            if(0 == Dir_Unlink(input)){
                printf("%s successfully removed.\n", input);
            } else {
                printf("%s removal failed.\n", input);
            }

            break;
        case(5) :


             printf("Path of directory to read? \n");            
             getString();
             int s = Dir_Size(input);


            if(0 >= s){
                printf("%s did not exist or was empty.\n", input);
            } else {
                char readbuffer[s/20][20];
                Dir_Read(input, readbuffer, s);
    
            }

            //free(readbuffer);

            break;
        case(6) :

             printf("Path of file to make? \n");            
             getString();

            if(0 == File_Create(input)){
                printf("%s successfully created.\n", input);
            } else {
                printf("%s creation failed.\n", input);
            }

            break;
        case(7) :

             printf("Path of file to remove? \n");            
             getString();

            if(0 == File_Unlink(input)){
                printf("%s successfully removed.\n", input);
            } else {
                printf("%s removal failed.\n", input);
            }

            break;
        case(8) :

            printf("Path of file to open? \n");            
            getString();
            fd = File_Open(input);

            if(fd >= 0){
                printf("%s opened with file descriptor %d.\n", input, fd);
            } else {
                printf("Failed to open %s.\n", input);
            }

            break;
        case(9) :


            printf("File descriptor to read from? \n");
            fd = getNum();
            printf("Name of file on stu? \n");            
            getString();
            printf("Number of bytes? \n");
            bytes = getNum();

            stu = fopen( input ,"wb+" );
            if (!stu) {
                printf("Unable to open file: %s\n", input);
            } else {

                buffer = (char *)malloc(bytes * sizeof(char));

                read = File_Read(fd, buffer, bytes);
                fwrite(buffer, sizeof(char), read, stu);

                if(read >= 0){
                    printf("Read %d bytes from fd %d.\n", read, fd);
                } else {
                    printf("Read from %d failed.\n", fd);
                }
            }
            free(buffer);
            fclose(stu);

            break;
        case(10) :

            printf("File descriptor to write to? \n");
            fd =  getNum();
            printf("Name of file on stu? \n");            
            getString();
            printf("Number of bytes? \n");
            bytes = getNum();



            stu = fopen( input ,"rb+" );  
            if (!stu) {
                printf("Unable to open file: %s\n", input);
            } else {

                buffer = (char *)malloc(bytes * sizeof(char));
                read = fread(buffer, sizeof(char), bytes, stu);

                int wrote = File_Write(fd, buffer, bytes);

                printf("This is the buffer: %s", buffer);


                if(wrote >= 0){
                    printf("Wrote %d bytes to fd %d.\n", wrote, fd);
                } else {
                    printf("Write to %d failed.\n", fd);
                }
            }

            free(buffer);
            fclose(stu);

            break;
        case(11) :

            printf("File descriptor to seek inside? \n");
            fd = getNum();      
            printf("Seek location? \n");
            seek = getNum();

            if(File_Seek(fd, seek) >= 0){
                printf("Seek operation succeded.\n");
            } else {
                printf("Seek failed.\n");
            }
            

            break;
        case(12) :

            printf("File descriptor to close? \n");
            fd = getNum();         

            if(File_Close(fd) >= 0){
                printf("Close operation succeded.\n");
            } else {
                printf("Close failed.\n");
            }

            break;
        case(13) :
            printf("Program will now exit \n");
            *end = true;

            break;
        //case(14) :

         //   break;

        default:

            printf("Entry should be a number between 1 and 13\n");
    

    } // end switch

}    // end menu

int getNum(){
    fgets(n, buffersize, stdin);
    int num = atoi(n);
    return num;
}

void getString(){


    fgets(input, buffersize, stdin);
    int t = strlen(input);
    if (t>0 && input[t-1]=='\n') { input[t-1] = 0; }

}

/*void printdirline(char *, int index){



}*/
