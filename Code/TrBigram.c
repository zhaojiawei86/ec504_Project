#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>

#include "TrBigram.h"

int ascii2Bigram(uint8_t *bigBuff, char *ascBuff, int n)
{
    uint16_t i = 0;
    uint8_t temp = 0;
    int j = 0;
    for(i = 0; i < n; i++)
    {
        if(ascBuff[i] <= 'z' && ascBuff[i] >= 'a')
            temp = (uint8_t)(ascBuff[i] - 'a' + BIGALFL);
        else if(ascBuff[i] <= 'Z' && ascBuff[i] >= 'A')
            temp = (uint8_t)(ascBuff[i] - 'A' + BIGALFU);
        else if(ascBuff[i] <= '9' && ascBuff[i] >= '0')
            temp = (uint8_t)(ascBuff[i] - '0' + BIGNUM);
        else
            temp = 0;   //Fill wil sapce
        buf_c = buf_c | ((temp<<(OUT_BYTE-SHORT)) >> rem);
        rem = rem + SHORT;
        if(rem >= OUT_BYTE)
        {
            bigBuff[j++] = buf_c;
            rem = rem % OUT_BYTE;
            buf_c = 0;
            buf_c = buf_c | (temp << (OUT_BYTE-rem));
        }
    }
    if(buf_c != 0)
        bigBuff[j++] = buf_c; 
    return j;
}

void Bigram2ascii(uint8_t *bigBuff, char *ascBuff, int n)
{
    uint16_t i = 0;
    uint8_t temp = 0;
    uint8_t buf = 0;
    int j = 0;
    for(i = 0; i < n; i++)
    {
        if (crem + SHORT >= OUT_BYTE)
        { 
            buf = (bigBuff[j++]<<crem);
            buf = buf >> (OUT_BYTE-SHORT);
            crem = (crem + SHORT) % OUT_BYTE;
            buf = buf | (bigBuff[j] >> (OUT_BYTE-crem));
        }
        else
        {
            buf = (bigBuff[j]<<crem) >> (OUT_BYTE-SHORT);
            crem += SHORT;
        }
        
        if(buf < BIGALFU && buf >= BIGALFL)
            buf = buf - BIGALFL + 'a';
        else if(buf < BIGNUM && buf >= BIGALFU)
            buf = buf - BIGALFU + 'A';
        else if(buf < 64 && buf >= BIGNUM)
            buf = buf - BIGNUM + '0';
        else 
            buf = ' ';

        ascBuff[i] = buf;
    }
}
int file_ascii2Bigram(char* asc_file_name, char* big_file_name)
{
    int count = 0;
    int count_c = 0;
    FILE *inFile;
    FILE *outFile;
    char buff[BUFFSIZE];
    uint8_t out[BUFFSIZE];
    size_t amount = 0;
    int amount_c = 0;
    inFile = fopen(asc_file_name, "r");
    outFile = fopen(big_file_name, "w");
    rem = 0;

    while((amount = fread(buff, sizeof(char), ASCSIZE, inFile)) > 0)
    {
        if(rem != 0)
            fseek(outFile, -1, SEEK_CUR);
        count += amount;
        amount_c = ascii2Bigram((uint8_t*)out, (char*)buff, amount);
        count_c += amount_c;
        fwrite(out, sizeof(uint8_t), amount_c, outFile);
    }
    fclose(outFile);
    fclose(inFile);
    return count_c;
}
int file_Bigram2ascii(char* asc_file_name, char* big_file_name)
{
    int count = 0;
    int count_c = 0;
    FILE *inFile;
    FILE *outFile;
    uint8_t buff[BUFFSIZE];
    char out[BUFFSIZE];
    size_t amount = 0;
    int amount_c = 0;
    outFile = fopen(asc_file_name, "w");
    inFile = fopen(big_file_name, "r");
    rem = 0;

    while((amount = fread(buff, sizeof(uint8_t), BIGSIZE, inFile)) > 0)
    {
        count += amount;
        Bigram2ascii((uint8_t*)buff, (char*)out, amount*LONG/SHORT);
        fwrite(out, sizeof(char), amount*LONG/SHORT, outFile);
        count_c += amount*LONG/SHORT;
    }
    fclose(outFile);
    fclose(inFile);
    return count_c;
}

int file_save_as(char* disk_Name, char* file_Name, char* disk_File_Name)
{
    FILE *aFile;
    int fID;
    int count = 0;
    mount_fs(disk_Name);
    file_ascii2Bigram(file_Name, "tempfile");
    aFile = fopen("tempfile", "r");
    fs_create(disk_File_Name);
    fID = fs_open(disk_File_Name);
    uint8_t buff[READSIZE];

    while((count = fread(buff, sizeof(uint8_t), READSIZE, aFile)) > 0)
        fs_write(fID, (void*)buff, count);

    fs_close(fID);
    fclose(aFile);
    umount_fs(disk_Name);
    remove("tempfile");
    return 0;
}
int file_load_as(char* disk_Name, char* file_Name, char* disk_File_Name)
{
    FILE *aFile;
    int fID;
    int count = 0;
    mount_fs(disk_Name);
    aFile = fopen("tempfile", "w");
    fID = fs_open(disk_File_Name);
    uint8_t buff[READSIZE];

    while((count = fs_read(fID, (void*)buff, READSIZE)) > 0)
        fwrite(buff, sizeof(uint8_t), count, aFile);
    
    fs_close(fID);
    fclose(aFile);
    umount_fs(disk_Name);
    file_Bigram2ascii(file_Name, "tempfile");
    remove("tempfile");
    return 0;
}
int file_load_as_coded(char* disk_Name, char* file_Name, char* disk_File_Name)
{
    FILE *aFile;
    int fID;
    int count = 0;
    mount_fs(disk_Name);
    aFile = fopen(file_Name, "w");
    fID = fs_open(disk_File_Name);
    uint8_t buff[READSIZE];

    while((count = fs_read(fID, (void*)buff, READSIZE)) > 0)
        fwrite(buff, sizeof(uint8_t), count, aFile);
    
    fs_close(fID);
    fclose(aFile);
    umount_fs(disk_Name);
    return 0;
}
