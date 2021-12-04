#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>

#include "fs.h"
#include "disk.h"
#include "TrBigram.h"

#define READSIZE 1024
#define DISKSIZE 33     //Mb

int main(int argc, char *argv[])
{
    char*** file_List = (char ***)malloc(sizeof(char***));
    int count = 0;
    int usedSpace = 0;      //Kb
    int fileSize = 0;
    int fID = 0;
    
    if(strcmp(argv[1], "-newdisk") == 0)
    {
        make_fs(argv[2]);
        printf("Created New Disk: %s \n", argv[2]);
    }
    else if(strcmp(argv[1], "-ls") == 0)
    {
        printf("Disk Content===============\n");
        mount_fs(argv[2]);
        fs_listfiles(file_List);
        while((*file_List)[count] != NULL)
        {
            fID = fs_open((*file_List)[count]);
            fileSize = fs_get_filesize(fID);
            usedSpace += fileSize;
            fs_close(fID);
            printf("%s ---- %d Bytes -- %d MBytes \n", (*file_List)[count], fileSize, fileSize/1000000);
            count++;
        }
        umount_fs(argv[2]);
        printf("\n\nDisk Usage: %d MB out of %d MB\n", usedSpace/1000000, DISKSIZE);
    }
    else if(strcmp(argv[1], "-save") == 0)
    {
        file_save_as(argv[2], argv[3], argv[4]);
        printf("File Saved\n");
    }
    else if(strcmp(argv[1], "-load") == 0)
    {
        file_load_as(argv[2], argv[3], argv[4]);
        printf("File Loaded\n");
    }
    else if(strcmp(argv[1], "-load_NoDecode") == 0)
    {
        file_load_as_coded(argv[2], argv[3], argv[4]);
        printf("File Loaded without Decoding\n");
    }
    else if(strcmp(argv[1], "-encode") == 0)
    {
        file_ascii2Bigram(argv[2], argv[3]);
        printf("File Encoded\n");
    }
    else if(strcmp(argv[1], "-decode") == 0)
    {
        file_Bigram2ascii(argv[2], argv[3]);
        printf("File Decoded\n");
    }
    else if(strcmp(argv[1], "-rm") == 0)
    {
        mount_fs(argv[2]);
        fs_delete(argv[3]);
        umount_fs(argv[2]);
        printf("File Deleted from Disk %s: %s\n", argv[2], argv[3]);
    }
    else
        printf("Unrecognized Command: %s\n", argv[1]);
    return 0;
}