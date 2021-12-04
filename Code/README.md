EC504 2021 Fall Project VFS DropBox-like storage on the Linux file system

	
The submission files contains a series of functions that can use Trained Bigram to encode/decode
files, create VFS disk and an UI to read, write and delete files from the VFS disk. Each VFS disk has
 a data space ~33MB and it is also the maximum space for a single file (maximum file number is 64).
The VFS is interacting with the disk in terms of blocks(4kb each block). When loading files out from 
the disk, the default function will automatically decode the file and returns the original file.

To use the UI, make the project and use the following command:

Creating a new disk with its name
./UI_Test -newdisk {DiskName}

Listing the content and usage of a disk
./UI_Test -ls {DiskName}

Saving a file to the disk
./UI_Test -save {DiskName} {OriginalFileName} {FileNameInVFS}

Deleting a file in the disk
./UI_Test -rm {DiskName} {FileNameInVFS}

Loading a file from the disk
./UI_Test -load {DiskName} {FileName} {FileNameInVFS}

Loading a file from the disk, without decoding
./UI_Test -load_NoDecode {DiskName} {FileName} {FileNameInVFS}

Encoding a file
./UI_Test -encode {ASCIIFileName} {TrainBigramFileName}

Decoding a file
./UI_Test -decode {ASCIIFileName} {TrainBigramFileName}


submitted files:
	TrBigram.c
	TrBigram.h
	UI_Test.c
	fs.c
	fs.h
	disk.c
	disk.h
	makefile
	README.md
	//Testing .txt files



