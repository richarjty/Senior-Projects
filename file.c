/*-----------------------------------------------------------------------------------------
PA3 - file.c

Library of functions for file manipulation

Written By   :    Jon Richardson
Version      :    November 5, 2020

-----------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "pa3.h"

extern bool mounted;
extern char *partition;
extern struct tableEntry fileTable[32];

int File_Create(char *dirpath) {

    int len = strlen(dirpath); // strtok, which I need to use frequently, destroys its input
    char path[len];            // Thus all of this garbage
    char path2[len];
    char path3[len];           // Its not my fault


    strcpy(path, dirpath);
    strcpy(path2, dirpath);
    strcpy(path3, dirpath);

    int parent;
    long int seek;
    FILE *f;

    parent = Dir_Parent(path);
    int checkinode = getInodeFromPath(path2);


    if (parent == -1) {
        printf("File_Create: Invalid file path: %s\n", dirpath);
        return -1;
    }
    if (checkinode != -1) {
        printf("File_Create: File already exists: %s\n", dirpath);
        return -1;
    }
    f = openPartition();
    if (!f) {
        return -1;
    }

    // Find free inode/data blocks
    int inode = getFreeInode();
    int data = getFreeDataBlock();

    if ((inode == -1) || (data == -1)){
        printf("File_Create: Could not locate free inode or data block: %s\n", dirpath);
        return -1;
    }

    // We have now found a free inode and data block
    // marking them as in use
    reserveInode(inode);
    reserveDataBlock(data);


    char *name = getName(path3);
    char allocname[16];
    strcpy(allocname, name);

    // apply information to parent directory 
    if (-1 == Dir_Allocate(parent, FIL, allocname, inode)) {
        fclose(f);
        return -1;
    }

    short int type = FIL;
    makeInode(inode, type, data);

    fclose(f);
    return 0;
}

int File_Open(char *dirpath) {

    int len = strlen(dirpath); // strtok, which I need to use frequently, destroys its input
    char path[len]; 


    strcpy(path, dirpath);


    int inode = getInodeFromPath(path);


    if (inode == -1) {
        printf("File_Open: Filepath is invalid: %s\n", dirpath);
        return -1;
    }

    int fd = getFreeFD();

    if (fd == -1) {
        printf("File_Open: No Available File Descriptors\n");
        return -1;
    }

    fileTable[fd].used = true;
    fileTable[fd].inode = inode;

    return fd;
}

int File_Read(int fd, void *buffer, int bytes) {

    if(fileTable[fd].used == false) {
        printf("File_Read: Given File Descriptor is not associated with an open file\n");       
        return -1;
    }

    int inode = fileTable[fd].inode;

    FILE *f = openPartition();
    long int seek;

    short info[3];
    char datablocks[26];

    readInode(inode, info, datablocks);

    int size = info[2];
    int read = 0;
    int offset = fileTable[fd].pointer;
    int block = 0;

    int toread = bytes;                // calculating how much we will read in advance
    if (toread > (size - offset)) {
        toread = size - offset;
    }

    while (offset >= SECTOR_SIZE){
        offset = offset - SECTOR_SIZE;    // offset then becomes offset into current block
        block++;                          // update block in parallel
    }

    int readhere = SECTOR_SIZE - offset;

    seek = SECTOR_SIZE*10;  // Data sectors start at 10
    seek = seek + SECTOR_SIZE*datablocks[block];
    seek = seek + offset; 
    fseek(f, seek, SEEK_SET);  


    while(read < toread) {

        if (readhere > (toread - read)) {
            readhere = (toread - read);
        }

        seek = SECTOR_SIZE*10;  // Data sectors start at 10
        seek = seek + SECTOR_SIZE*datablocks[block];
        seek = seek + offset;
        fseek(f, seek, SEEK_SET);
        
        fread(buffer, sizeof(char), readhere, f);

        read = read + readhere;
        fileTable[fd].pointer = fileTable[fd].pointer + readhere;

        readhere = SECTOR_SIZE;
        offset = 0;
        block++;
    } 
    
    fclose(f);
    return read;
}

int File_Write(int fd, void *buffer, int bytes) {

    if(fileTable[fd].used == false) {
        printf("File_Write: Given File Descriptor is not associated with an open file\n");       
        return -1;
    }

    int inode = fileTable[fd].inode;

    FILE *f = openPartition();
    long int seek;

    short info[3];
    char datablocks[26];

    readInode(inode, info, datablocks);
    int size = info[2];
    int towrite = bytes;
    int written = 0;
    int block = 0;

    int offset = fileTable[fd].pointer;
    while (offset >= SECTOR_SIZE){
        offset = offset - SECTOR_SIZE;    // offset then becomes offset into current block
        block++;                          // update block in parallel
    }

    int writehere = SECTOR_SIZE - offset;
    bool quit = false;

    seek = SECTOR_SIZE*10;  // Data sectors start at 10
    seek = seek + SECTOR_SIZE*datablocks[block];
    seek = seek + offset;  
    fseek(f, seek, SEEK_SET); 

    //printf("Seek: %ld Written: %d Towrite %d WRitehere: %d \n", seek, written, towrite, writehere );

    while((written < towrite) && !quit) {

        if (block == 26) {
            printf("File_Write: Exceeded maximum file size\n");
            info[2] = size + written;
            writeInode(inode, info, datablocks);
            fclose(f);
            fileTable[fd].pointer = fileTable[fd].pointer + written;
            return written;
        }

        if (writehere > (towrite - written)) {
            writehere = (towrite - written);
        }

        if(datablocks[block] == 0) {                // allocate new datablock for writes
            int newblock = getFreeDataBlock();
            if (newblock == -1) {
                printf("File_Write: Unable to secure new datablock\n");
                info[2] = size + written;
                writeInode(inode, info, datablocks);
                fileTable[fd].pointer = fileTable[fd].pointer + written;
                fclose(f);
                return written;
            }
        
            datablocks[block] = newblock;
        }

        seek = SECTOR_SIZE*10;  // Data sectors start at 10
        seek = seek + SECTOR_SIZE*datablocks[block];
        seek = seek + offset;  
        fseek(f, seek, SEEK_SET); 

        fwrite(buffer, sizeof(char), writehere, f);
             

        written = written + writehere;    
        writehere = SECTOR_SIZE;
        offset = 0;
        block++;
    }
    
    fileTable[fd].pointer = fileTable[fd].pointer + written;
    info[2] = size + written;
    writeInode(inode, info, datablocks);
    fclose(f);
    return written;
}

int File_Seek(int fd, int offset) {

    if(fileTable[fd].used == false) {
        printf("File_Seek: Given File Descriptor is not associated with an open file\n");       
        return -1;
    }

    int inode = fileTable[fd].inode;
    int size = Dir_SizeFromInode(inode);

    if(offset >= size) {
        printf("File_Seek: Offset is out of range for file\n");       
        return -1;
    }
    if(offset < 0) {
        printf("File_Seek: Offset cannot be negative\n");       
        return -1;
    }

    fileTable[fd].pointer = offset;

    return offset;    // weird to return an input but ok
}

int File_Close(int fd) {

    if(fileTable[fd].used == false) {
        printf("File_Close: Given File Descriptor is not associated with an open file\n");       
        return -1;
    }    

    fileTable[fd].used = false;
    fileTable[fd].pointer = 0;
    fileTable[fd].inode = -1;

    return 0;
}

int File_Unlink(char *dirpath) {

    int len = strlen(dirpath); // strtok, which I need to use frequently, destroys its input
    char path[len];
    char path2[len];

    strcpy(path, dirpath);
    strcpy(path2, dirpath);

    int inode = getInodeFromPath(path);

    for(int i = 0; i < 32; i++) {
        if (fileTable[i].inode == inode) {
            printf("File_Unlink: Cannot unlink an open file\n");
            return -1;
        }
    }

    int ret = Dir_Unlink(path2);

    return ret;
}

int File_getOffset(int size) {
    int offset = size;
    while (offset >= SECTOR_SIZE){
        offset = offset - SECTOR_SIZE;
    }
    return offset;
}
