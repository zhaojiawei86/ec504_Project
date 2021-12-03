#ifndef _TR_Bigram_
#define _TR_Bigram_

/******************************************************************************/
#define SHORT       6      /* number of bits for A-Z, a-z, 0-9, space         */
#define LONG        8      /* number of bits for all other ASCII              */
#define OUT_BYTE    8      /* number of bits buffer for Bigram*/  
#define BIGBIFFSIZE 4096      /* buffer size for Bigram         */
#define ASCBIFFSIZE 4096/2      /* buffer size for ASCII         */
#define BIGSPACE    0
#define BIGALFL     1
#define BIGALFU     27
#define BIGNUM      53
#define BIGFLAG     64 // NOT APPLIED
#define BUFFSIZE    600
#define BIGSIZE     360
#define ASCSIZE     480
#define READSIZE    1024

/******************************************************************************/
int ascii2Bigram(uint8_t *bigBuff, char *ascBuff, int n);    /* convert ASCII to Trained Bigram       */
void Bigram2ascii(uint8_t *bigBuff, char *ascBuff, int n);       /* convert Trained Bigram to ASCII     */

int file_ascii2Bigram(char* asc_file_name, char* big_file_name);
int file_Bigram2ascii(char* asc_file_name, char* big_file_name);
int file_save_as(char* disk_Name, char* file_Name, char* disk_File_Name);
int file_load_as(char* disk_Name, char* file_Name, char* disk_File_Name);
int file_load_as_coded(char* disk_Name, char* file_Name, char* disk_File_Name);
/******************************************************************************/

static int rem = 0;
static int crem = 0;
static uint8_t buf_c = 0;

//TR_Bigram coding:
//0:        space
//1-26:     a-z
//27-52:    A-Z
//53-63:    0-9
//64:       Flag (NOT APPLIED)
#endif