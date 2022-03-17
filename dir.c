/*-----------------------------------------------------------------------------------------
PA3 - dir.c

Library of functions for directory manipulation

Written By   :    Jon Richardson
Version      :    November 5, 2020

-----------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "pa3.h"
//#include "util.c"

extern char *partition;

int Dir_Create(char *dirpath){
    //printf("DIRCREATE method \n");
    
    char path[strlen(dirpath)];
    strcpy(path, dirpath);

    int parent;
    long int seek;
    FILE *f;

    parent = Dir_Parent(path);
    //printf("DIRCREATE after parent method. Parent was %d\n", parent);

    if (parent == -1) {
        printf("Dir_Create: Invalid file path: %s\n", dirpath);
        return -1;
    }
    f = openPartition();
    if (!f) {
        return -1;
    }


    // Find free inode/data blocks
    int inode = getFreeInode();
    int data = getFreeDataBlock();

    //printf("DIRCREATE after finding free blocks. Blocks were %d and %d\n", inode, data);

    if ((inode == -1) || (data == -1)){
        printf("Dir_Create: Could not locate free inode or data block: %s\n", dirpath);
        return -1;
    }

    // We have now found a free inode and data block
    // marking them as in use
    reserveInode(inode);
    reserveDataBlock(data);

    char path2[strlen(dirpath)];
    strcpy(path2, dirpath);
    char *name = getName(path2);
    char allocname[16];
    strcpy(allocname, name);    


    //printf("DIRCREATE attempting to allocate %s \n", allocname);
    // apply information to parent directory 
    if (-1 == Dir_Allocate(parent, DIR, allocname, inode)) {
        fclose(f);
        return -1;
    }

    short int type = DIR;
    makeInode(inode, type, data);
    
    //printf("DIRCREATE after writing into this directory's inode\n");
    //short info[3];
    //char datablocks[26];
    //readInode(1, info, datablocks);
    //printInodeData(info, datablocks);
    //printf("Dir size is now %d\n", info[2]); 
    
    // In theory thats it
    
    fclose(f);
    return 0;
}

int Dir_Size(char *dirpath){

    char path[strlen(dirpath)];
    strcpy(path, dirpath);

    int inode = getInodeFromPath(path);
    if (inode == -1) {
        printf("Dir_Size: Invalid filepath\n");
        return -1;
    }
    int ret = Dir_SizeFromInode(inode);
    
    return ret;
}

int Dir_Read(char *path, void *buffer, int size){

    char path2[strlen(path)];
    strcpy(path2, path);

    //printf("Path: %s\n", path2);

    int inode = getInodeFromPath(path2);
    short info[3];
    char datablocks[26];

    //printf("Reading Dir at INODE: %d\n", inode);

    readInode(inode, info, datablocks);

    FILE *f = openPartition();
    long int seek;


    if ( size < info[2]){
         printf("Dir_Read: Insufficient bytes to receive directory\n");
         return -1;
    }

    int n = size / 20;            // number of entries (size/20)
    int r = 0;                    // number read
    int block = 0;                // current block of data

    char namebuffer[16];
    int inodebuffer;

    while (r < n ) {
        seek = SECTOR_SIZE*(datablocks[block]+10);  
        fseek(f, seek, SEEK_SET);
        for (int i = 0; (i < 25) && (r < n); i ++){
            fread(namebuffer, sizeof(char), 16, f);
            fread(&inodebuffer, sizeof(int), 1,  f);

            printf("Name: %s  Inode %d \n", namebuffer, inodebuffer);

            //fwrite(namebuffer, sizeof(char), 16, buffer);
            //fwrite(&inodebuffer, sizeof(int), 1,  buffer);
            r++;
        }
        block++;
    }

/*
    int success = 0;
    fseek(f, seek, SEEK_SET);

    for (int i = 0; i < 25; i++){
        if( 16 != fread(names[i], sizeof(char), 16, f)) success = -1;
        if( 1  != fread(&inodes[i], sizeof(int), 1,  f)) success = -1;

        //if (strlen(names[i]) > 0) {
        //    printf("READ DIR DATA Name: %s and inode %d \n", names[i], inodes[i]);
        //}
    }
  */  


    return r; // number of entries in directory
}


int Dir_Unlink(char *dirpath){

    int len = strlen(dirpath);
    char path[len];
    char path2[len];
    char path3[len];
    char name[16];
    strcpy(path, dirpath);
    strcpy(path2, path);
    strcpy(path3, path);

    int parent = Dir_Parent(path);            // parent's inode that is
    int inode = getInodeFromPath(path2);
    int size = Dir_SizeFromInode(parent);
    int sizechild = Dir_SizeFromInode(inode);
    char *tname = getName(path3);
    strcpy(name, tname);
    

    if (parent == -2)  {
        printf("Dir_Unlink: Invalid request - cannot delete root\n");
        return -1;
    }
    if (sizechild != 0) {
        printf("Dir_Unlink: Invalid request - cannot delete a directory that is not empty\n");
        return -1;
    }
    if (parent == -1)  {
        printf("Dir_Unlink: Invalid request - Bad filepath\n");
        return -1;
    }

    short info[3];
    char datablocks[26];
    readInode(parent, info, datablocks);


    int n = size / 20;            // number of entries (size/20)
    int r = 0;                    // number read
    int block = 0;                // current block of data
    bool found = false;

    char names[25][16];
    int  inodes[25];

    // read through all datablocks looking for token - remove it
    // and write each entry one slot down  

    // save location of last place to write to
    FILE *f = openPartition();
    int lastblock;
    int lastindex; 

    //printf("Searching for %s inside dir of inode %d \n", name, parent);

    while (r < n ) {
        Dir_ReadDirectoryData(datablocks[block], names, inodes);
        for (int i = 0; (i < 25) && (r < n); i ++){
            //printf("Comparing %s with %s\n", token, names[i]);

            //printf("Comparing %s with %s\n", name, names[i]);

            if (0 == strcmp(name, names[i])){
                //printf("%s was found to be identical to %s at index %d\n", name, names[i], i);
                found = true;
            }

            if (found){
                lastblock = block;
                lastindex = i;
            }
            
            r++;
        }
        block++;
    }
    // Need one more repetition to clear last entry
    if (found){
        char blank[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 
        Dir_WriteDirectoryEntry(datablocks[lastblock], lastindex, blank, 0);
    }


    fclose(f);
    if (!found) {
        printf("%s was not found inside parent directory\n", name);
        return -1;
    } 

    Dir_ChangeSize(parent, (size - 20));
    return 0;
}

// change directory's size to size (in bytes)
int Dir_ChangeSize(int inode, short size){

    long int seek;
    FILE *f = openPartition();
    seek = SECTOR_SIZE+(inode*32)+4; // get to size value of directory inode
    fseek(f, seek, SEEK_SET);
    short int newsize = size;
    //printf("Dir_ChangeSize method seeking to %ld \n", seek);
    //printf("Dir_ChangeSize: writing new size: %d\n", size);

    if (0 > fwrite(&newsize, sizeof(short), 1, f)) {
        printf("Dir_ChangeSize: Unable to write to file: %s\n", partition);
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}

// Return size given an inode instead of path
int Dir_SizeFromInode(int inode){


    long int seek;
    unsigned short size;

    seek = ((SECTOR_SIZE)+(32*inode)+4);

    FILE *f = openPartition();
    fseek(f, seek, SEEK_SET);
    if (1 > fread(&size, sizeof(unsigned short), 1, f)) {
        printf("Dir_Size: Failed to read file.\n");
        fclose(f);
        return -1;
    }

    int ret = size;
    fclose(f);
    return ret;
}

// Takes a path and returns the inode of parent directory, or -1 if the path is invalid, or -2 if the path
// provided is the root - which is a valid path but has no parent
// Will still return inode of parent if last element does not exist
int Dir_Parent(char *dirpath) {
    //printf("DIRPARENT method \n");


    //printf("Length of dirpath is %ld \n", strlen(dirpath));
    char path[100];
    strcpy(path, dirpath);
    //printf("Length of path is %ld \n", strlen(path));

    // early exit for root directory
    if (0 == strcmp(path, "/")) {
        //printf("Size of root: %d\n", currentSize);
        //printf("But root has no parent!\n");
        return -2; // specific inode for root
    }

    //------------------------------------------------------------

    // begin at root
    int parent = 0;
    char *rootstring = "root";
    const char ch[2] = "/";
    char temp[strlen(dirpath)];
    strcpy(temp, path);

    char strings[144][16];
    int parents[144];

    char *token = strtok(temp, ch);

    parents[0] = parent;
    strcpy(strings[0], rootstring);
    int count = 1;
    bool quit = false;


    while (token != NULL){ // && !quit){ // while there remain tokens and tokens exist as file/dir

        //printf("Token %d: %s \n",count, token);
 
        strcpy(strings[count], token);                               // add strings at count
        int node = getInode(strings[count], parents[count-1]);       // add parent at count
        parents[count] = node;
        token = strtok(NULL, ch);                                    // refresh token

        //if (node == -1) {quit = true;}


        count++;
    }

    //printf("We examined %d tokens and the last one had name %s with inode %d \n", count-1, strings[count-1], parents[count-1]);

    // print off the array
    //for (int i = 0; i < count; i++) {
    //    printf("Count: %d String: %s Inode %d \n", i, strings[i], parents[i]);
    //}

    // now set parent
    parent = parents[count-2];

    //printf("Parent of %s was determined to have inode %d\n", strings[count-1], parent);
    
    return parent;
}





// takes a datablock index, and then reads the entire block into provided arrays
int Dir_ReadDirectoryData(int sector, char names[25][16], int inodes[25]) {
    //printf("DIRREADDIRECTORYDATA method \n");

    FILE *f = openPartition();
    long int seek;
    //seek = sector*SECTOR_SIZE;
    seek = SECTOR_SIZE*(sector+10);  // Data sectors start at 10

    int success = 0;
    fseek(f, seek, SEEK_SET);

    for (int i = 0; i < 25; i++){
        if( 16 != fread(names[i], sizeof(char), 16, f)) success = -1;
        if( 1  != fread(&inodes[i], sizeof(int), 1,  f)) success = -1;

        //if (strlen(names[i]) > 0) {
        //    printf("READ DIR DATA Name: %s and inode %d \n", names[i], inodes[i]);
        //}
    }

    fclose(f);
    return success;
}

// Read a single entry in a specific spot
// Sector is index of directory, which is actual sector index + 10
int Dir_ReadDirectoryEntry(int sector, int index, char name[16], int *inode) {

    FILE *f = openPartition();
    long int seek;
    seek = SECTOR_SIZE*(sector+10);  // Directory sectors start at 10
    seek = seek + (20*index);

    int success = 0;
    fseek(f, seek, SEEK_SET);
    
    if(0 > fread(name, sizeof(char), 16, f)) success = -1;
    int temp;
    if(0 > fread(&temp, sizeof(int), 1, f)) success = -1;
    *inode = temp;    

    fclose(f);
    return success;
}

// Write a single entry to a specific spot
// Sector is index of directory, which is actual sector index + 10
int Dir_WriteDirectoryEntry(int sector, int index, char name[16], int inode) {

    FILE *f = openPartition();
    long int seek;
    seek = SECTOR_SIZE*(sector+10);  // Directory sectors start at 10
    seek = seek + (20*index);

    int success = 0;
    fseek(f, seek, SEEK_SET);
    
    if(0 > fwrite(name, sizeof(char), 16, f)) success = -1;
    if(0 > fwrite(&inode, sizeof(int), 1, f)) success = -1;

    return success;
}

// given a parent inode, enter child information
int Dir_Allocate(int parent, short int type, char name[16], char inode) {
    //printf("DIRALLOC method start\n");

    long int seek;
    FILE *f;
    f = openPartition();
    if (!f) {
        return -1;
    }
    
    short info[3];
    char datablocks[26];
    readInode(parent, info, datablocks);
    short int size = info[2];

    if (size == 1300) {
        printf("Dir_Allocate: Directory already at max size.\n");
        return -1;
    }

    bool full = ( size > 0 && size%500 ==0); // determine if we are adding to a dir with full datablock

    char lastblock = 0;
    bool found = false;
    if (full) {

        // find first empty datablock slot in inode
        for (int i = 0; !found && (i < 26);i++){
            if (datablocks[i] == -1) {
                lastblock = i;
                found = true;
            }
        }
        seek = SECTOR_SIZE+(parent*32)+6; // get to datablock array of parent inode
        seek = seek + lastblock;                  // add index of block
        fseek(f, seek, SEEK_SET);
        // get a new block for this directory
        char newblock = getFreeDataBlock();
        if (0 > fwrite(&newblock, sizeof(char), 1, f)) {
            printf("Dir_Alloc: Unable to write to file: %s\n", partition);
            fclose(f);
            return -1;
        }
        lastblock = newblock;    // lastblock becomes newly assigned block
    } else {

        int tempsize = size;
        int index = 0;
        while (tempsize >= 500) {
            tempsize = tempsize - 500;
            index++;
        }
        lastblock = datablocks[index];   // lastblock becomes last valid block in array;
    }

    // determined if we need a new data block, and where we need to write to

  
    // int offset for how far into datablock we need to write

    short int offset = size;
    size = size + 20;

    while (offset >= 500) {
        offset = offset - 500;
    }

    // update size of Directory in inode

    Dir_ChangeSize(parent, size); // TODO verify this works

    // Inode should be correct, now add to data block

    // lastblock is where we are writing
    //printf("DIRALLOC method updating data in block of index %d\n", lastblock);
    seek = SECTOR_SIZE*(lastblock+10);        // seeking to datablock
    seek = seek + offset;                  // seeking to offset within block
    //printf("DIRALLOC method seeking to %ld \n", seek);
    fseek(f, seek, SEEK_SET);
    //printf("DIRALLOC method writing %s \n", name);
    char tempname[16];
    strcpy(tempname, name);

    if (0 > fwrite(tempname, sizeof(char), 16, f)) {
        printf("Dir_Alloc: Unable to write to file: %s\n", partition);
        fclose(f);
        return -1;
    }
    int tinode = inode;
    if (0 > fwrite(&tinode, sizeof(int), 1, f)) {
        printf("Dir_Alloc: Unable to write to file: %s\n", partition);
        fclose(f);
        return -1;
    }

    // Parent Inode updated and Datablock filled with child info (Theoretically)

    fclose(f);
    return 0;
}








