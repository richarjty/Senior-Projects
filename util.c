/*-----------------------------------------------------------------------------------------
PA3 - util.c

Library of functions for utility

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
struct tableEntry fileTable[32];

int format(char *filename){


    FILE *f;

    f = fopen( filename ,"wb+" );  // open binary file for write/read
    if (!f) {
        printf("Format: Unable to open file: %s\n", filename);
        return -1;
    }

    // Set size of file
    // This code writes a 0 to the last byte of the partition, forcing allocation
    // of data for its required size. Then it returns to the beginning of file.
    long int seek;
    seek = PARTITION_SIZE - 1;
    fseek(f, seek, SEEK_SET);
    fputc(0, f);
    fseek(f, 0 , SEEK_SET);

    // Write a fresh superblock
    unsigned int super[4];
    super[0] = MAGIC;         // Magic number - becomes LEAF as a string
    super[1] = 1;             // size of superblock, in sectors
    super[2] = 9;             // number of sectors containing inodes
    super[3] = 246;           // number of sectors containing data blocks

    if (0 > fwrite(&super, sizeof(unsigned int), 4, f)) {
        printf("Format: Unable to write to file: %s\n", filename);
        return -1;
    }

    char inodes[144];
    for (int i = 1; i < 144; i++) {
        inodes[i] = 0;
    }

    char data[246];
    for (int i = 1; i < 246; i++) {
        data[i] = 0;
    }

    // root data
    inodes[0] = 1;    // mark as in use
    data[0] = 1;
    
    if (0 > fwrite(&inodes, sizeof(char), 144, f)) {
        printf("Format: Unable to write to file: %s\n", filename);
        return -1;
    }
    
    if (0 > fwrite(&data, sizeof(char), 246, f)) {
        printf("Format: Unable to write to file: %s\n", filename);
        return -1;
    }

    // edit inode for root - do not need to edit data (yet)
    // seek to beginning of inodes
    seek = (1*SECTOR_SIZE);
    fseek(f, seek, SEEK_SET);
    unsigned short inodeData [3];
    inodeData[0] = INUSE;            // In use or free
    inodeData[1] = DIR;              // Directory or file
    inodeData[2] = 0;                // Size
    char dataBlocks [26];
    dataBlocks [0] = 0;
    for (int i = 1; i < 26; i++) {
        dataBlocks[i] = -1;
    }

    if (0 > fwrite(&inodeData, sizeof(unsigned short), 3, f)) {
        printf("Format: Unable to write to file: %s\n", filename);
        return -1;
    }

    if (0 > fwrite(&dataBlocks, sizeof(char), 26, f)) {
        printf("Format: Unable to write to file: %s\n", filename);
        return -1;
    }   

    fclose(f);

    return 0;
}

int mount(char *filename){

    //printf("Received %s\n", filename);

    // mounted is a global variable created in main in pa3.c
    if (mounted) {
        printf("File system already mounted\n");
        return -1;
    }


    FILE *f;
    f = fopen( filename ,"rb" );  // open binary file for read
    if (!f) {
        printf("Mount: Unable to open file: %s\n", filename);
        return -1;
    }

    unsigned int magic;
    if (1 > fread(&magic, sizeof(unsigned int), 1, f)) {
        printf("Mount: Failed to read magic number: %s\n", filename);
        return -1;
    }

    int ret = -1;

    if (magic == MAGIC){

        //strcpy(partition, filename);
        makeFileTable();
        ret = 0;

    }

    return ret;
}

// read in inode data given the index of inode
void readInode(char inode, short info[3], char datablocks[26]) {
    /* Use this to read in - must provide inode variable
    short info[3];
    char datablocks[26];

    readInode(inode, info, datablocks);
    */

    //printf("READINODE method: reading inode %d \n", inode);

    long int seek;
    FILE *f = openPartition();
    seek = SECTOR_SIZE + (inode*32);

    int inforead;
    int datablocksread;

    fseek(f, seek, SEEK_SET);
    inforead = fread(info, sizeof(unsigned short), 3, f);
     //   printf("readInode: Error reading inode (info)\n");
   
    datablocksread = fread(datablocks, sizeof(char), 26, f);
     //   printf("readInode: Error reading inode (datablocks)\n");

    //printf("READINODE method read %d info and %d datablocks\n", inforead, datablocksread);
   
    
    fclose(f);

}

// Write information into a specified inode
int writeInode(int inode, short int inodedata[3], char datalocations[26]){

    FILE *f = openPartition();
    long int seek; 
    
    // write this directory's inode
    seek = (SECTOR_SIZE) + (32*inode); // inodes are 32 bytes
    fseek(f, seek, SEEK_SET);
    if (0 > fwrite(inodedata, sizeof(short int), 3, f)) {
        printf("writeInode: Unable to write to file: %s\n", partition);
        fclose(f);
        return -1;
    }
    if (0 > fwrite(datalocations, sizeof(char), 26, f)) {
        printf("writeInode: Unable to write to file: %s\n", partition);
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}

// Make new inode in specified slot of specified type
int makeInode(int inode, short int type, int datablock){

    short int inodedata[3];
    inodedata[0] = INUSE;
    inodedata[1] = type;
    inodedata[2] = 0;        // size

    char datalocations[26];
    datalocations[0] = datablock;
    for (int i = 1; i < 26; i++){
        datalocations[i] = -1;
    } 
    
    writeInode(inode, inodedata, datalocations);

    return 0;
}

// read in inode data given the index of inode
void printInodeData(short info[3], char datablocks[26]) {

    printf("Short Data:");
    printf("%x %x %d", info[0], info[1], info[2]);
    printf("\n");

    printf("Datablocks:");
    for (int i =0; i < 26; i++){
        printf("%d ", datablocks[i]);
    }
    printf("\n");
}

int getFreeInode(){

    long int seek;
    FILE *f = openPartition();
    if (!f) {
        return -1;
    }

    seek = 16;    // beginning of inode blocks
    unsigned char inodes[144];

    // Search for an empty inode and data block
    fseek(f, seek, SEEK_SET);
    if( 144 > fread(inodes, sizeof(char), 144, f)){
        printf("getFreeInode: Failed to read inode bits: %s\n", partition);
        fclose(f);
        return -1;
    }

    char inode = -1;
    for (int i = 0; i < 144; i++) {
        if(inodes[i] == 0) {
            inode = i;
            i = 250; // just ending loop
        }
    }


    if (inode == -1) {
        printf("getFreeInode: Unable to find free inode: %s\n", partition);
        fclose(f);
        return -1;
    }
    fclose(f);
    return inode;
}

int getFreeDataBlock(){
    
    long int seek;
    FILE *f = openPartition();
    if (!f) {
        return -1;
    }

    seek = 16 + 144;    // beginning of data blocks
    unsigned char datablocks[246];

    // Search for an empty inode and data block
    //printf("getFreeDataBlock: Seeking to : %ld\n", seek);
    fseek(f, seek, SEEK_SET);

    if( 246 > fread(datablocks, sizeof(char), 246, f)){
        printf("getFreeDataBlock: Failed to read datablock bits: %s\n", partition);
        fclose(f);
        return -1;
    }


    char data = -1;

    for (int i = 0; i < 246; i++) {
    //printf("getFreeDataBlock: Data at %ld plus %d was : %d\n", seek, i, datablocks[i] );
        if(datablocks[i] == 0) {
            data = i;
            i = 250; // just ending loop
        }
    }


    if (data == -1) {
        printf("getFreeDataBlock: Unable to find free data block: %s\n", partition);
        fclose(f);
        return -1;
    }
    fclose(f);
    return data;

}

int reserveInode(int block){

    FILE *f = openPartition();

    char one = 1;
    long int seek = 16 + block;
    fseek(f, seek, SEEK_SET);

    if (0 > fwrite(&one, sizeof(char), 1, f)) {
        printf("Reserve_Inode: Unable to write to file: %s\n", partition);
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;

}

int reserveDataBlock(int block){

    FILE *f = openPartition();

    char one = 1;
    long int seek = 16 + 144 + block;
    fseek(f, seek, SEEK_SET);

    if (0 > fwrite(&one, sizeof(char), 1, f)) {
        printf("Reserve_Datablock: Unable to write to file: %s\n", partition);
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;

}

int freeInode(int block){

    FILE *f = openPartition();

    char zero = 0;
    long int seek = 16 + block;
    fseek(f, seek, SEEK_SET);

    if (0 > fwrite(&zero, sizeof(char), 1, f)) {
        printf("Reserve_Inode: Unable to write to file: %s\n", partition);
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;

}

int freeDataBlock(int block){

    FILE *f = openPartition();

    char zero = 0;
    long int seek = 16 + 144 + block;
    fseek(f, seek, SEEK_SET);

    if (0 > fwrite(&zero, sizeof(char), 1, f)) {
        printf("Reserve_Datablock: Unable to write to file: %s\n", partition);
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;

}

// Get the inode of a directory given its path
int getInodeFromPath(char *path) {

    int len = strlen(path);
    char path2[len];
    char path3[len];
    strcpy(path2, path);
    strcpy(path3, path);

    char *name = getName(path2);
    int parent = Dir_Parent(path3);
    int inode = getInode(name, parent);
    return inode;

}

// Get the inode of the directory matching token inside the directory associated with a given inode
int getInode(char *tokenin, int inode) {
    //printf("GETINODE method: %s inside directory inode %d \n", tokenin, inode);


    if (inode == -1) {
        //printf("GetInode early exit!\n");
        return -1;
    }
    if (inode == -2) {
        return 0;
    }

    int len = strlen(tokenin);
    char token[len];
    strcpy(token, tokenin);

    short int size;

    //printf("GETINODE method about to seek to %ld \n", seek);

    short info[3];
    char datablocks[26];

    readInode(inode, info, datablocks);
    //printInodeData(info, datablocks);

    size = info[2];

    //printf("GETINODE method after reading parent inode \n");

    // At this point we have the given inode's size and locations of
    // blocks holding its data
    


    int n = size / 20;            // number of entries (size/20)
    int r = 0;                    // number read
    int block = 0;                // current block of data
    bool found = false;

    char names[25][16];
    int  inodes[25];

    // read through all datablocks looking for token - Done!

    //printf("THIS IS R: %d AND N: %d \n", r, n);    

    while (r < n ) {

        Dir_ReadDirectoryData(datablocks[block], names, inodes);
        for (int i = 0; (i < 25) && (r < n); i ++){
            //printf("Comparing %s with %s\n", token, names[i]);
            if (0 == strcmp(token, names[i])){
                return inodes[i];
                //found = true;
            }
            r++;
        }
        block++;
    }



    //printf("GETINODE method after searching for token-name match of %s \n", token);
    
    // so if we finish the for-loop without finding it, it wasn't there (theoretically)
    //printf("Path lost at %s token.\n", token);

    return -1;
}

// Open the partition and return fd
FILE *openPartition(){
    FILE *f;
    f = fopen( partition ,"rb+" );  // open binary file for write/read
    if (!f) {
        printf("Open Partition: Unable to open file: %s\n", partition);
        return NULL;
    }
    return f;
}

// Given a path, return the name of the item without the path (the last item)
char *getName(char *dirpath) {

    char path[strlen(dirpath)];
    strcpy(path, dirpath);

    const char ch[2] = "/";
    char *token = strtok(path, ch);
    char *name = token;
    while (token != NULL){
        token = strtok(NULL, ch);
        if (token != NULL){
            name = token;
        }
    }

    return name;
}

void makeFileTable(){

    for(int i = 0; i < 32; i++) {
        fileTable[i].used = false;
        fileTable[i].pointer = 0;
        fileTable[i].inode = -1;
    }
}

// return a free FD
int getFreeFD(){

    for(int i = 0; i < 32; i++) {
        if ( fileTable[i].used == false) { // Yes I know. This way is more readable
            return i;
        }
    }
    return -1;
}
