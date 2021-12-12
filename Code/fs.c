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

#define DIR_BLC_NUM     20
#define INDIR_BLC_NUM   40
#define UNDEFINED       9999
#define PAGE_SIZE       250
#define MAX_DATA_BLOCK  8000
#define BYTE_SIZE       8
#define MAX_FILF        35000000
#define BITMAP_BLOCK    4096    //1 block is enough
#define MAX_FILE_NUM    64
#define FILE_NAME_LEN   16      //last for '\0'
#define MAX_OP_FILE     32

#define DIR_C           0
#define FILE_C          1
#define SUPER_B         0
#define BITMAP_B        1
#define INODE_B         2
#define DATA_B          75

struct dir_entry
{
    bool is_used;
    uint8_t inode_num;  // + offset get

};

struct super_block
{
    uint16_t used_block_bitmap_count;   //total block in used
    uint16_t used_block_bitmap_offset;  //location of the bitmap
    uint16_t inode_metadata_blocks;     //total number of inode
    uint16_t inode_metadata_offset;     //beginning of inode
    uint16_t data_block_offset;         //location startpoint of data
    char file_name[MAX_FILE_NUM][FILE_NAME_LEN];

};

//**All the inode offset initialize to MAX_NUM, start at 0
struct inode
{
    int8_t file_type;       //DIR or File
    uint16_t inode_num;      //Corresponding inode, add to offset
    uint16_t dir_blc[DIR_BLC_NUM];   //direct block
    uint16_t indir_blc[INDIR_BLC_NUM];  //indirect block
    uint32_t file_size;     //file size in Byte
    uint16_t block_num;
};

struct file_descriptor
{
    bool is_used;
    uint16_t inode_number;
    int offset;
};

static struct super_block *Sblock;
static unsigned int bitmap_offset = 0;
static unsigned int inode_offset = 0;
static unsigned int data_offset = 0;
static bool mounted = false;
static struct file_descriptor files_f[MAX_OP_FILE];

//load the bitmap into a buffer
//buffer need to be multiple block size
static void load_bitmap(uint8_t *buffer)
{
    block_read(bitmap_offset, (void*)buffer);
}
//save the bitmap using a buffer
static void save_bitmap(uint8_t *buffer)
{
    block_write(bitmap_offset, (void*)buffer);
}
//Check if a specific bit is 1/0
static bool check_bitmap_bit(uint8_t *bitmap ,int block_offset)
{
    int element = block_offset/BYTE_SIZE;
    int offset = block_offset%BYTE_SIZE;
    if(bitmap[element] == (bitmap[element] | (1<<offset)))
        return true;
    return false;
}
//Set a bit in the bitmap to be 1
static bool set_bitmap_bit(uint8_t *bitmap ,int block_offset)
{
    int element = block_offset/BYTE_SIZE;
    int offset = block_offset%BYTE_SIZE;
    bitmap[element] = (bitmap[element] | (1<<offset));
    Sblock->used_block_bitmap_count += 1;
}
//Set a bit in the bitmap to be 1
static bool free_bitmap_bit(uint8_t *bitmap ,int block_offset)
{
    int element = block_offset/BYTE_SIZE;
    int offset = block_offset%BYTE_SIZE;
    bitmap[element] = (bitmap[element] & (~(1<<offset)));
    Sblock->used_block_bitmap_count -= 1;
}
//Get the block index of a specific byte in the file
static unsigned int get_inode_bnum(unsigned int byte_num)
{
    return byte_num/BLOCK_SIZE;
}
//return the data block offset of given inode and inode block num
static uint16_t get_inode_block_offset(struct inode *node, int inode_bnum)
{
    int page_num = (inode_bnum - DIR_BLC_NUM)/PAGE_SIZE;
    int page_offset = (inode_bnum - DIR_BLC_NUM)%PAGE_SIZE;
    uint16_t *temp;
    uint16_t b_offset = UNDEFINED;
    //locate in direct block
    if (inode_bnum < DIR_BLC_NUM)
        return node->dir_blc[inode_bnum];
    //locate in indirect block
    if (node->indir_blc[page_num] == UNDEFINED)
        return UNDEFINED;
        
    temp = (uint16_t *)malloc(BLOCK_SIZE);
    if(block_read((node->indir_blc[page_num]) + data_offset, (void*)temp) == -1)
    {
        free(temp);
        perror("can not read indirect page");
        exit(0);
    }
    b_offset = temp[page_offset];
    free(temp);
    return b_offset;
}
//always set the next data block of the inode
static int set_inode_block_offset(struct inode *node, uint16_t block_offset)
{
    int inode_bnum = node->block_num;
    int page_num = (inode_bnum - DIR_BLC_NUM)/PAGE_SIZE;
    int page_offset = (inode_bnum - DIR_BLC_NUM)%PAGE_SIZE;
    uint16_t *temp;
    int i = 0;
    int ii = 0;
    uint8_t *bitmap;
    //locate in direct block
    if (inode_bnum < DIR_BLC_NUM)
    {
        node->dir_blc[inode_bnum] = block_offset;
        node->block_num += 1;
        return 0;
    }
    //locate in indirect block
    if (node->indir_blc[page_num] == UNDEFINED)
    {
        bitmap = malloc(BITMAP_BLOCK);
        load_bitmap(bitmap);
        for(i = 0; i < MAX_DATA_BLOCK; i++)
            if(check_bitmap_bit(bitmap, i) == false)
            {
                set_bitmap_bit(bitmap, i);
                save_bitmap(bitmap);
                node->indir_blc[page_num] = i;
                free(bitmap);
                break;
            }
    }
        
    temp = (uint16_t *)malloc(BLOCK_SIZE);
    if(block_read((node->indir_blc[page_num]) + data_offset, (void*)temp) == -1)
    {
        free(temp);
        perror("can not read indirect page");
        exit(0);
    }
    if(page_offset == 0)    //if the first element
        for(ii = 0; ii < PAGE_SIZE; ii++)
            temp[ii] = UNDEFINED;
    temp[page_offset] = block_offset;
    node->block_num += 1;
    block_write((node->indir_blc[page_num]) + data_offset, (void*)temp);
    free(temp);
    return 0;
}
//extent the given file (inode) by 1 block, return the data block offset
//return UNDEFINED when no space left
static uint16_t set_1block(struct inode *node)
{
    uint16_t add_iblock = node->block_num;
    uint8_t *bitmap;
    uint16_t j = 0;
    if (node->file_size > MAX_FILF-BLOCK_SIZE)
        return UNDEFINED;

    bitmap = malloc(BITMAP_BLOCK);
    load_bitmap(bitmap);
    for(j = 0; j < MAX_DATA_BLOCK; j++)
        if(check_bitmap_bit(bitmap, j) == false)
        {
            set_bitmap_bit(bitmap, j);
            save_bitmap(bitmap);
            free (bitmap);
            set_inode_block_offset(node, j);
            return j;
        }
    //no free space
    save_bitmap(bitmap);
    free(bitmap);
    return UNDEFINED;
}
//delete 1 block from the given file (inode)
static int free_1block(struct inode* node)
{
    if(node->block_num <= 0)
    {
        perror("already 0 block");
        exit(0);
    }
    int block_offset = 0;
    uint16_t *temp;
    uint8_t *bitmap = (uint8_t *)malloc(BITMAP_BLOCK);
    load_bitmap(bitmap);
    block_offset = get_inode_block_offset(node, (node->block_num)-1);
    free_bitmap_bit(bitmap, block_offset);
    save_bitmap(bitmap);
    free(bitmap);
    //set the inode
    node->block_num -= 1;// index of last block
    if (node->block_num < DIR_BLC_NUM)
    {
        node->dir_blc[node->block_num] = UNDEFINED;
        return 0;
    }
    //free one in indirect part
    int page_num = (node->block_num - DIR_BLC_NUM)/PAGE_SIZE;
    int page_offset = (node->block_num - DIR_BLC_NUM)%PAGE_SIZE;
    temp = malloc(BLOCK_SIZE);
    block_read(data_offset + node->indir_blc[page_num], temp);
    temp[page_offset] = UNDEFINED;
    block_write(data_offset + node->indir_blc[page_num], temp);
    free(temp);
    //if the end of 1 indirect block
    if( page_offset == 0)
    {
        bitmap = malloc(BITMAP_BLOCK);
        load_bitmap(bitmap);
        free_bitmap_bit(bitmap, node->indir_blc[page_num]);
        save_bitmap(bitmap);
        free(bitmap);
        node->indir_blc[page_num] = UNDEFINED;
    }
    return 0;
}
//Create a disk with a given name
int make_fs(const char *disk_name)
{
    if(make_disk(disk_name) != 0)
        return -1;
    open_disk(disk_name);
    //SUPER block
    int counti = 0; 
    Sblock = (struct super_block*)malloc(BLOCK_SIZE);
    Sblock->used_block_bitmap_count = 0;
    Sblock->used_block_bitmap_offset = BITMAP_B;
    Sblock->inode_metadata_blocks = 0;
    Sblock->inode_metadata_offset = INODE_B;
    Sblock->data_block_offset = DATA_B;
    for(counti = 0; counti < MAX_FILE_NUM; counti++)
        Sblock->file_name[counti][0] = '\0';
    block_write(0, Sblock);
    free(Sblock);
    //bitmap
    uint8_t *bitmap = malloc(BLOCK_SIZE);
    int countj = 0;
    for(countj = 0; countj < MAX_DATA_BLOCK/BYTE_SIZE; countj++)
        bitmap[countj] = 0;
    block_write(BITMAP_B, Sblock);
    free(bitmap);
    //inode
    int countk = 0, countkk = 0, countkkk = 0;
    struct inode* temp_node = malloc(BLOCK_SIZE);
    temp_node->block_num = 0;
    for(countk = 0; countk<DIR_BLC_NUM; countk++)
        temp_node->dir_blc[countk] = UNDEFINED;
    for(countkk = 0; countkk<INDIR_BLC_NUM; countkk++)
        temp_node->indir_blc[countkk] = UNDEFINED;
    temp_node->file_size = 0;
    temp_node->file_type = FILE_C;
    for(countkkk = 0; countkkk<MAX_FILE_NUM; countkkk++)
    {
        temp_node->inode_num = countkkk;
        block_write(INODE_B + countkkk, temp_node);
    }
    free(temp_node);

    close_disk();
    return 0;
}
//Mount a disk with the disk name
int mount_fs(const char *disk_name)
{
    int a;
    if(open_disk(disk_name) != 0)
        return -1;
    Sblock = (struct super_block*)malloc(BLOCK_SIZE);
    if(block_read(0, Sblock) != 0)
        return -1;
    if(Sblock->data_block_offset != DATA_B)
        return -1;
    data_offset = Sblock->data_block_offset;
    bitmap_offset = Sblock->used_block_bitmap_offset;
    inode_offset = Sblock->inode_metadata_offset;
    mounted = true;
    for(a = 0; a<MAX_OP_FILE; a++)
        files_f[a].is_used = false;
    return 0;
}
//Unmount the disk
int umount_fs(const char *disk_name)
{
    if(mounted != true)
        return -1;
    if(block_write(0, Sblock) != 0)
        return -1;
    free(Sblock);
    mounted = false;
    close_disk(disk_name);
    return 0;
}
//Open a file in the disk, return the file identifier number
int fs_open(const char *name)
{
    int b = 0, c = 0;
    uint16_t inode_num = UNDEFINED;
    for(c = 0; c<MAX_FILE_NUM; c++)
        if(strcmp(name, Sblock->file_name[c]) == 0)
            inode_num = (uint16_t)c;
    //if not found
    if (inode_num == UNDEFINED)
        return -1;
    for(b = 0; b < MAX_OP_FILE; b++)
        if(files_f[b].is_used == false)
            break;
    //if ALL USING
    if (b == MAX_OP_FILE)
        return -1;

    files_f[b].inode_number = inode_num;
    files_f[b].is_used = true;
    files_f[b].offset = 0;
    return b;
}
//Close a file with its identifier number
int fs_close(int fd)
{
    if(fd >= MAX_OP_FILE)
        return -1;
    if(files_f[fd].is_used == false)
        return -1;
    files_f[fd].inode_number = UNDEFINED;
    files_f[fd].offset = 0;
    files_f[fd].is_used = false;
    return 0;
}
//Create a new file in the disk
int fs_create(const char *name)
{
    int t;
    int checkl;
    if(strlen(name)>15)
        return -1;
    for(t = 0; t < MAX_FILE_NUM; t++)
        if(strcmp(name, Sblock->file_name[t]) == 0)
            return -1;  //already exit
    if(Sblock->inode_metadata_blocks > MAX_FILE_NUM)
        return -1;
    for(t = 0; t< MAX_FILE_NUM; t++)
        if(Sblock->file_name[t][0] == '\0')
        {
            strcpy(Sblock->file_name[t], name);
            Sblock->file_name[t][strlen(name)] = '\0';
            Sblock->inode_metadata_blocks += 1;
            return 0;
        }
    return -1;//unexpected
}
//Delete a file in the disk
int fs_delete(const char *name)
{
    int g = 0, countg = 0;
    uint16_t inode_num = UNDEFINED;
    struct inode *temp;
    //should not be opening
    for(g = 0; g < MAX_OP_FILE; g++)
        if(files_f[g].is_used == true)
            if(strcmp(Sblock->file_name[files_f[g].inode_number], name) == 0)
                return -1;
    for(g = 0; g<MAX_FILE_NUM; g++)
        if(strcmp(name, Sblock->file_name[g]) == 0)
            inode_num = (uint16_t)g;
    if (inode_num == UNDEFINED)
        return -1;
    //clearing the content
    temp = (struct inode*)malloc(BLOCK_SIZE);
    block_read(inode_offset + inode_num, temp);
    countg = temp->block_num;
    for (g = 0; (g < countg)&&(temp->block_num > 0); g++)
        if(free_1block(temp) != 0)
            perror("can not free block");
    temp->file_size = 0;
    block_write(inode_offset + inode_num, temp);
    free(temp);
    //
    Sblock->file_name[g][0] = '\0';
    Sblock->inode_metadata_blocks -= 1;
    int l = 0;
    for(l = 0; l < MAX_FILE_NUM; l++)
        if(strcmp(name, Sblock->file_name[l]) == 0)
            strcpy(Sblock->file_name[l], "");
    return 0;
}
//Read n bytes of data from the file into the buffer
int fs_read(int fd, void *buf, size_t nbyte)
{
    if(files_f[fd].is_used == false)
        return -1;
    int target = files_f[fd].offset + nbyte;
    int amount = 0;
    int current_block = files_f[fd].offset/BLOCK_SIZE;
    int current_boffset = files_f[fd].offset%BLOCK_SIZE;
    struct inode *node_a;
    void *buffer;
    node_a = (struct inode *)malloc(BLOCK_SIZE);
    buffer = (void *)malloc(BLOCK_SIZE);
    block_read(inode_offset + files_f[fd].inode_number, node_a);
    if (target > node_a->file_size)
        target = node_a->file_size;
    int target_block = target/BLOCK_SIZE;
    int target_boffset = target%BLOCK_SIZE;
    for (current_block = files_f[fd].offset/BLOCK_SIZE;
        current_block <= target_block; current_block++)
    {
        block_read(data_offset + get_inode_block_offset(node_a, current_block),
                    (void*)buffer);
        if (current_block == target_block)
            memcpy((void*)&(buf[amount]),
                    (void*)&(buffer[current_boffset]), target_boffset-current_boffset);
        else
            memcpy((void*)&(buf[amount]),
                    (void*)&(buffer[current_boffset]), BLOCK_SIZE-current_boffset);
        current_boffset = 0; //offset = 0 for other than first block
        amount += BLOCK_SIZE-current_boffset;
    }
    amount = target - files_f[fd].offset;
    files_f[fd].offset = target;
    free(node_a);
    free(buffer);
    return amount;
}
//Write n bytes of data into the file from the buffer
int fs_write(int fildes, void *buf, size_t nbyte)
{
    if(files_f[fildes].is_used == false)
        return -1;
    int target = files_f[fildes].offset + nbyte;
    int amount = 0;
    int current_block = files_f[fildes].offset/BLOCK_SIZE;
    int current_boffset = files_f[fildes].offset%BLOCK_SIZE;
    struct inode *node_a;
    void *buffer;
    node_a = (struct inode *)malloc(BLOCK_SIZE);
    buffer = (void *)malloc(BLOCK_SIZE);
    block_read(inode_offset + files_f[fildes].inode_number, node_a);
    if (target > MAX_FILF)
        target = MAX_FILF;
    int target_block = target/BLOCK_SIZE;
    int target_boffset = target%BLOCK_SIZE;
    for (current_block = files_f[fildes].offset/BLOCK_SIZE;
        current_block <= target_block; current_block++)
    {
        if(amount + files_f[fildes].offset > MAX_FILF)
            break;
        //need to extend
        if(get_inode_block_offset(node_a, current_block) == UNDEFINED)
            if(set_1block(node_a) == UNDEFINED)
                break; //no more space
    
        block_read(data_offset + get_inode_block_offset(node_a, current_block),
                    (void*)buffer);
        if (current_block == target_block)
        {
            memcpy((void*)&(buffer[current_boffset]),
                    (void*)&(buf[amount]), target_boffset-current_boffset);
            amount += target_boffset-current_boffset;
        }
        else
        {
            memcpy((void*)&(buffer[current_boffset]),
                    (void*)&(buf[amount]), BLOCK_SIZE-current_boffset);
            amount += BLOCK_SIZE-current_boffset;
        }
        current_boffset = 0; //offset = 0 for other than first block
        block_write(data_offset + get_inode_block_offset(node_a, current_block),
                    (void*)buffer);
    }
    if(node_a->file_size < (amount + files_f[fildes].offset))
        node_a->file_size = amount + files_f[fildes].offset;
    block_write(inode_offset + files_f[fildes].inode_number, node_a);
    files_f[fildes].offset += amount;
    free(buffer);
    free(node_a);
    return amount;
}
//Get the size of a file
int fs_get_filesize(int fd)
{
    int size;
    struct inode *node;
    if(files_f[fd].is_used == false)
        return -1;
    node = malloc(BLOCK_SIZE);
    block_read(inode_offset + files_f[fd].inode_number, node);
    size = node->file_size;
    free(node);
    return size;
}
//List all existing file and their names
int fs_listfiles(char ***files)
{
    if(mounted == false)
        return -1;
    int p = 0;
    int countp = 0;
	*files = (char **)malloc(sizeof(char*) * MAX_FILE_NUM);
    for(p = 0; p < MAX_FILE_NUM; p++)
        if(Sblock->file_name[p][0] != '\0')
            (*files)[countp++] = strdup(Sblock->file_name[p]);
	(*files)[countp] = NULL;
    return 0;
}
//Seek to a specific place in a file
int fs_lseek(int fd, off_t offset)
{
    if(files_f[fd].is_used == false)
        return -1;
    if(offset < 0)
        return -1;
    if(offset > fs_get_filesize(fd))
        return -1;
    files_f[fd].offset = offset;
    return 0;
}
//Truncate a file at a specific offset
int fs_truncate(int fd, off_t length)
{
    struct inode *node;
    int count = 0;
    if(files_f[fd].is_used == false)
        return -1;
    node = malloc(BLOCK_SIZE);
    block_read(inode_offset + files_f[fd].inode_number, node);
    if(length > node->file_size)
    {
        free(node);
        return -1;
    }
    if(files_f[fd].offset > length)
        files_f[fd].offset = length;
    int target_block = length/BLOCK_SIZE;
    for(count = (node->block_num) - 1; count > target_block; count--)
        free_1block(node);
    node->file_size = length;
    block_write(inode_offset + files_f[fd].inode_number, node);
    free(node);
    return 0;
}
