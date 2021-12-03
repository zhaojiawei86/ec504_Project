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

int main()
{
    int fID;
    //Create a disk, do nothing if disk is already exist
    make_fs("DISK2");
    //Save 10M-03.txt into DISK2 with name 10M
    file_save_as("DISK2", "10M-03.txt", "10M");
    //Conver 10M-03.txt using trained Bigram coding, this step is outside the disk
    file_ascii2Bigram("10M-03.txt", "10M-03_coded");
    //Load 10M from DISK2 with decoding, using name 10M-03_load.txt, 10M-03_load.txt should be the same as 10M-03.txt
    file_load_as("DISK2", "10M-03_load.txt", "10M");
    //Load 10M from DISK2 without decoding, using name 10M-03_load_coded, 10M-03_load_coded should be the same as 10M-03_coded
    file_load_as_coded("DISK2", "10M-03_load_coded", "10M");
    //check the file size, should return the same size as 10M-03_coded
    mount_fs("DISK2");
    fID = fs_open("10M");
    printf("Compressed File in Disk is %d Bytes\n", fs_get_filesize(fID));
    fs_close(fID);
    umount_fs("DISK2");

    return 0;
}