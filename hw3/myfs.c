#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>


#define FAT_ENTRY_COUNT 4096
#define FILELIST_START_OFFSET 16384 
#define FILELIST_ENTRY_COUNT 128
#define FILELIST_ENTRY_SIZE 256
#define DATA_OFFSET (FILELIST_START_OFFSET + FILELIST_ENTRY_COUNT * FILELIST_ENTRY_SIZE)
#define CHUNK_SIZE 512

#define intFrom4Byte(chr) (*(chr) | *(chr+1) << 8 | *(chr+2)  << 16 | *(chr+3) << 24)

void format(FILE* disk_fp);
void fs_write(FILE* disk_fp, char *srcPath, int lenSrcPath ,char *destFileName, int lenDestFileName);
void fs_read(FILE* disk_fp, char *srcFileName,int lenSrcFileName, char *destPath, int lenDestPath);
void fs_delete(FILE* disk_fp, char *filename);
void fs_list(FILE * disk_fp);

void fileList_print(FILE* disk_fp);
void fileList_setEntry(FILE* disk_fp, char *destFileName,int lenDestFileName,  int firstBlock, int fileSize);
void fileList_getFirstBlock(FILE* disk_fp, char* fileName, int* blockIndex, int* fileSize, int* filelist_index);
void fileList_deleteEntry(FILE* disk_fp, int index);

void fat_print();
void fat_setEntryValue(FILE* disk_fp ,int value, int entry);
int fat_getEmptyBlockEntry(FILE * disk_fp);
int fat_getValueFromEntry(FILE * disk_fp, int entry);

void defragment();

void inttobytes(int num, char* bytes);

int main( int argc, char *argv[] ){
    if(argc < 3){
        printf("Invalid number of arguments\n");
        exit(0);
    }

    char * disk_name = argv[1];
    FILE * disk = fopen(disk_name, "r+");
    
    if(disk == NULL){
        printf("ERROR: Disk can not open or not found");
        exit(0);
    }
    
    if(strcmp(argv[2], "-format") == 0 ){
        format(disk);
    }
    else if(strcmp(argv[2], "-write") == 0 ){
        fs_write(disk, argv[3], strlen(argv[3]), argv[4], strlen(argv[4]));
    }
    else if(strcmp(argv[2], "-read") == 0 ){
        fs_read(disk, argv[3], strlen(argv[3]), argv[4], strlen(argv[4]));
    }
    else if(strcmp(argv[2], "-delete") == 0 ){
        fs_delete(disk, argv[3]);
    } 
    else if(strcmp(argv[2], "-list") == 0 ){
        fs_list(disk);
    }
    else if(strcmp(argv[2], "-printfilelist") == 0 ){
        fileList_print(disk);
    }
    else if(strcmp(argv[2], "-printfat") == 0){
        fat_print(disk);
    }
    else{
        printf("Error: Invalid argument %s\n", argv[2]);
        exit(0);
    }

    
    /*fileList_getFirstBlock(disk, "testcp.txt", 10, &blockIndex, &srcFileSize);
    printf("%d %d", blockIndex, srcFileSize);
*/
    //fs_delete(disk,"testcp.txt");
    //fs_read(disk, "testcp.txt", 10, "result.txt", 10);
    /*fs_write(disk, "test.txt",8, "testcp1.txt",11);
    fs_write(disk, "test.txt",8, "testcp2.txt",11);
    fs_write(disk, "test.txt",8, "testcp3.txt",11);
*/
  //  fs_delete(disk,"testcp2.txt");
    
    //fat_setEntryValue(disk ,1, 1);
    //fat_getEmptyBlockEntry(disk);
    fclose(disk);
    /*
    for(int i = 0; i < 1024; i++){
        buf[i] = 'a';
    }
    fwrite(buf, 10, 1 , disk);

    fread(buf, 1024,1, disk);
    buf[1023] = '\0';
    for(int i = 0; i < 1024; i++){
        printf("%x", buf[i]);
    }
    -*/
}   

void format(FILE* disk_fp){
    /*set file cursor to beginning of file */ 
    fseek(disk_fp, 0 ,SEEK_SET);
    /*crete empty buffer to format headers*/
    char empty_buff[DATA_OFFSET];
    memset(empty_buff, 0x0,sizeof(empty_buff));
    /*for (int i = 0; i < 4096; i++)
    {
        empty_buff[i] = 0x0;
    }*/
    

    /*first entry of FAT is 0xffffffff */
    empty_buff[0] = 0xff;
    empty_buff[1] = 0xff;
    empty_buff[2] = 0xff;
    empty_buff[3] = 0xff;
    fwrite(empty_buff, sizeof(empty_buff), 1, disk_fp);
}

void fs_write(FILE* disk_fp, char *srcPath, int lenSrcPath ,char *destFileName, int lenDestFileName){
    
    FILE * srcFile_fp  = fopen(srcPath ,"r");
    /*find file size*/
    fseek(srcFile_fp, 0, SEEK_END);
    int srcFile_size = ftell(srcFile_fp);
    fseek(srcFile_fp, 0, SEEK_SET);
    //printf("src file size: %d \n", srcFile_size);
    /**/ 

    char buffer[CHUNK_SIZE];
    int srcFile_cursor = 0;
    int prevEmptyBlockEntry = 0, startBlockEntry;
    bool getStartBlockEntryFlag = false;
    
    while(srcFile_cursor < srcFile_size){
        srcFile_cursor += CHUNK_SIZE;
        /*read buffer*/
        fread(buffer, sizeof(buffer), 1, srcFile_fp);

        int emptyBlockEntry = fat_getEmptyBlockEntry(disk_fp); //get empty entry in FAT 
        printf("write location: %d\n",DATA_OFFSET + emptyBlockEntry * CHUNK_SIZE );
        fseek(disk_fp, DATA_OFFSET + emptyBlockEntry * CHUNK_SIZE, SEEK_SET); //go to target address in data
        
        fwrite(buffer, sizeof(buffer),1, disk_fp); // write target address
        
        if(prevEmptyBlockEntry != 0){
            fat_setEntryValue(disk_fp, emptyBlockEntry, prevEmptyBlockEntry);
        }
        if(getStartBlockEntryFlag == false){
            getStartBlockEntryFlag = true;
            startBlockEntry = emptyBlockEntry;
        }      
        prevEmptyBlockEntry = emptyBlockEntry;
    }   
    
    fileList_setEntry(disk_fp,destFileName, lenDestFileName, startBlockEntry, srcFile_size);
}

void fs_read(FILE* disk_fp, char *srcFileName,int lenSrcFileName, char *destPath, int lenDestPath){
    
    FILE *dest_fp;
    int blockIndex = 0;
    int srcFileSize = 0;
    int filelistIndex = 0;
    //get first block of desired file with its name
    fileList_getFirstBlock(disk_fp, srcFileName, &blockIndex, &srcFileSize, &filelistIndex);
    if(blockIndex != -1){
        dest_fp = fopen(destPath, "w");
    }else{
        printf("File not found\n");
        exit(0);
    }


    int copiedSize = 0;
    char buffer[CHUNK_SIZE];
    printf("%d %d", srcFileSize, copiedSize);
    do 
    {
        int address = blockIndex * CHUNK_SIZE + DATA_OFFSET;
        
        fseek(disk_fp, address, SEEK_SET);
        fread(buffer, sizeof(buffer), 1 , disk_fp);
        if(srcFileSize - copiedSize >= CHUNK_SIZE){
            fwrite(buffer, sizeof(buffer), 1 , dest_fp);    
            copiedSize += CHUNK_SIZE;
        }
        else{
            
            fwrite(buffer, srcFileSize - copiedSize, 1 , dest_fp);
            copiedSize += srcFileSize - copiedSize;
        }
        blockIndex   = fat_getValueFromEntry(disk_fp, blockIndex);
    }while(blockIndex != 0xffffffff);
    
   fclose(dest_fp);
   
}

void fs_delete(FILE* disk_fp, char *filename){
    int blockIndex = 0;
    int nextBlockIndex = 0;
    int srcFileSize = 0;
    int fileListIndex = 0;
    fileList_getFirstBlock(disk_fp, filename, &blockIndex, &srcFileSize, &fileListIndex);
    printf("%d", fileListIndex);
    fileList_deleteEntry(disk_fp, fileListIndex);

    do
    {   
        nextBlockIndex = fat_getValueFromEntry(disk_fp, blockIndex);
        fat_setEntryValue(disk_fp, 0x00,blockIndex);
        blockIndex = nextBlockIndex; 
    } while (nextBlockIndex != 0xffffffff);
    
}

void fs_list(FILE * disk_fp){
    fseek(disk_fp, FILELIST_START_OFFSET, SEEK_SET);
    char buff[FILELIST_ENTRY_SIZE];
    
    for(int i = 0; i < FILELIST_ENTRY_COUNT ; i++){
        fread(buff, sizeof(buff), 1, disk_fp);
        if(!(buff[0] == '.' || buff[0] == 0x0))
        {
            printf("%s\n", buff);
        }   
    }
}

void fat_setEntryValue(FILE* disk_fp ,int value, int entry){
    fseek(disk_fp, entry * 4 , SEEK_SET);
    fwrite(&value,4,1, disk_fp);
}

int fat_getEmptyBlockEntry(FILE * disk_fp){
    fseek(disk_fp, 0, SEEK_SET);
    int value;

    for (int entry = 0; entry < FAT_ENTRY_COUNT; entry++)
    {
        size_t size = fread(&value, 4, 1, disk_fp);
        //printf("cursor. %ld\n", ftell(disk_fp));
        //printf("%x , %zu\n", value,size);
        if(value == 0x0){
            fat_setEntryValue(disk_fp, 0xffffffff, entry);
    
            return entry;
        }
    }
}

int fat_getValueFromEntry(FILE * disk_fp, int entry){
    int value = 0;
    fseek(disk_fp, entry * 4, SEEK_SET);
    fread(&value, sizeof(int), 1 ,disk_fp);
    return value;
}

void fat_print(FILE* disk_fp){
    FILE* fp = fopen("fat.txt","w");
    fprintf(fp,"Entry\tValue\t\tEntry\tValue\t\tEntry\tValue\t\tEntry\tValue\n");
    
    fseek(disk_fp, 0, SEEK_SET);
    uint8_t row[16];
    for (int i = 0; i < FAT_ENTRY_COUNT / 4; i++)
    {
        fread(row, sizeof(row), 1, disk_fp);
        for (int j = 0; j < 4; j++)
        {
            fprintf(fp, "%04d\t0x%02x%02x%02x%02x\t", i * 4 + j, row[4* j],row[4* j +1 ],row[4*j+2],row[4*j+3]);
        }
        fprintf(fp, "\n");
    }
    
    
}

void fileList_setEntry(FILE* disk_fp, char *destFileName,int lenDestFileName,  int firstBlock, int fileSize)
{

    uint8_t fileListEntry[FILELIST_ENTRY_SIZE];
    int address = 0;
    fseek(disk_fp, FILELIST_START_OFFSET, SEEK_SET);
    for(int i = 0; i < FILELIST_ENTRY_COUNT; i++){
        /*get entry*/ 
        fread(fileListEntry, FILELIST_ENTRY_SIZE, 1, disk_fp);
        printf("list entry %s\n",fileListEntry);
        if(fileListEntry[0] == 0x0){
            address = FILELIST_START_OFFSET + i * FILELIST_ENTRY_SIZE; 
            break;
        }        
    }
    printf("file list target address: %d\n", address);
    fseek(disk_fp, address, SEEK_SET);
    /*create entry */
    memcpy(fileListEntry, destFileName, lenDestFileName);
    fileListEntry[247] = '\0';
    char byte_arr[4];

    inttobytes(firstBlock, byte_arr);
    memcpy(fileListEntry + 248, &firstBlock, 4);
    inttobytes(fileSize, byte_arr);
    memcpy(fileListEntry + 252, &fileSize, 4);
    /*printf("%u ", fileSize);
    fileListEntry[252] = byte_arr[0];
    fileListEntry[253] = byte_arr[1];
    fileListEntry[254] = byte_arr[2];
    fileListEntry[255] = byte_arr[3];

    printf("252 .%u\n" ,fileListEntry[252]);
    for (int i = 0; i < 256; i++)
    {
        printf("xxx %u\n", fileListEntry[i]);
    }
    printf("xxxx");
    */
    /*write*/
    fwrite(fileListEntry, FILELIST_ENTRY_SIZE, 1, disk_fp);    
}

void fileList_deleteEntry(FILE* disk_fp, int index){
    printf("reset filelist addr: %d\n", FILELIST_START_OFFSET + index * FILELIST_ENTRY_SIZE);
    fseek(disk_fp, FILELIST_START_OFFSET + index * FILELIST_ENTRY_SIZE, SEEK_SET );
    char buff[FILELIST_ENTRY_SIZE];
    for (int  i = 0; i < 256; i++)
    {
        buff[i] = 0x0;
    }
    
    fwrite(buff, 256, 1, disk_fp);
}

void fileList_getFirstBlock(FILE* disk_fp, char* fileName, int* blockIndex, int* fileSize, int* filelist_index){
    fseek(disk_fp, FILELIST_START_OFFSET, SEEK_SET);
    uint8_t entry[FILELIST_ENTRY_SIZE];

    for (int i = 0; i < FILELIST_ENTRY_COUNT; i++)
    {
        fread(entry, sizeof(entry),1 ,disk_fp);
        if(strcmp(entry, fileName) == 0)
        {           
           *(blockIndex) = intFrom4Byte(entry + 248);
            *(fileSize) = intFrom4Byte(entry + 252);

            *(filelist_index) = i;
            return;
        }
    }
    /* file not found in file table */ 
    *blockIndex = -1;
    *fileSize = -1 ;   
}

void fileList_print(FILE* disk_fp){
    FILE *print_fp = fopen("filelist.txt", "w");
    fseek(disk_fp, FILELIST_START_OFFSET, SEEK_SET);
    uint8_t entry[256];
    fprintf(print_fp, "Item\tFile name\tFirst Block\t\tFile size (Bytes)\n");
    for (int i = 0; i < FILELIST_ENTRY_COUNT; i++)
    {
        fread(entry, sizeof(entry), 1 ,disk_fp);
        int first_block = intFrom4Byte(entry + 248);
        int file_size = intFrom4Byte(entry + 252);
        fprintf(print_fp, "%03d\t%s\t\t\t%d\t\t\t%d\n",i, entry,first_block, file_size);
    }
    fclose(print_fp);
}

void inttobytes(int num, char* bytes){
    printf("inttonye %x\n", num&0xff);
    *(bytes) = num & 0xff;
    *(bytes+1) = (num >> 8) & 0xff;
    *(bytes+2) = (num >> 16) & 0xff;
    *(bytes+3) = (num >> 24) & 0xff;
}