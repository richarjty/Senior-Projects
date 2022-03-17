/*-----------------------------------------------------------------------------------------
PA3 - pa3.h

Header File 

Written By   :    Jon Richardson
Version      :    November 6, 2020

-----------------------------------------------------------------------------------------*/


#define SECTOR_SIZE      512
#define NUM_SECTORS      256
#define FREE             0x0000
#define INUSE            0x1111
#define FIL 		 0x2222
#define DIR 		 0x3333

#define MAGIC            0x4c454146          
#define PARTITION_SIZE   SECTOR_SIZE*NUM_SECTORS
#define NUM_INODES       144
#define NUM_DATA         246



typedef struct tableEntry {
    bool used;
    //bool open;
    int inode;
    long int pointer;
} table;

extern bool mounted;
extern char *partition;
extern struct tableEntry fileTable[32];

// Main driver
void menu (bool*);
int getNum();
void getString();
//void printdirline();

// Util
int format( char* );
int mount( char* );

void readInode(char, short[3], char[26]);
int writeInode(int, short int[3], char[26]);
int makeInode(int, short int, int);
void printInodeData(short[3], char[26]);

int getFreeInode();
int getFreeDataBlock();
int reserveInode(int);
int reserveDataBlock(int);
int freeInode(int);
int freeDataBlock(int);

int getInodeFromPath(char *);
int getInode(char *, int);
FILE *openPartition();
char *getName(char *);

void makeFileTable();
int getFreeFD();

// Dir
int Dir_Create(char *);
int Dir_Size(char *);
int Dir_Read(char *, void *, int);
int Dir_Unlink(char *);

int Dir_ChangeSize(int, short);
int Dir_SizeFromInode(int);
int Dir_Parent(char *);
int Dir_ReadDirectoryData(int, char[25][16], int[25]);
int Dir_ReadDirectoryEntry(int, int, char[16], int *);
int Dir_WriteDirectoryEntry(int, int, char[16], int);
int Dir_Allocate(int, short int, char[], char);

// File
int File_Create(char *);
int File_Open(char *);
int File_Read(int, void *, int);
int File_Write(int, void *, int);
int File_Seek(int, int);
int File_Close(int);
int File_Unlink(char *);

