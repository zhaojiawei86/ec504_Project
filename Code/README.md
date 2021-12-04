EC440 -HW5 VFS virtual file system implemented on the Linux file system
U68417996 lzy2022@bu.edu

	The submission files contains a series of functions that can set up a VFS and provide APIs 
to read and write files. The VFS has a data space ~30MB and it is also the maximum space for a 
single file (maximum file number is 64). User can create and access the files using the filename 
and file descripter (maximun 36 file descripter at the same time). The VFS is interacting with the
disk in terms of blocks(4kb each block), and the SuperBlock structure is stored in the first block
on the disk. Each inode contains 20 direct block storage and 40 single indeirct block storage, which
can allocate more than 40MB storage. All the inode IO is stored on the first 3~67 (total 64 inodes) blocks.
The inode is read and writen back whenever the program is doing file IO. The bit map is also writen back
whenever new block is being allocated or freed. So any matadate is keep updated and saved from accidental
system failure.
	The bitmap is located on the 2nd block of the disk and stored as uint_8 array (length is <max block # / 8>).
The bit map is accessed bitwisely through bitwise & and | operation to find the correct block to allocate/free. 
	To implement 40MB data storage, one way (although I did not use) I can come up with is to put a 
"shell" on the provided disk read/write function. If the program is accessing a block with # larger than
max block # of a single disk,the shelled disk read/write function should close the current disk, open the 
other disk (disk B), read/write on the other disk with <offset = accessing # - max block #>. After finishing the IO,
the shelled function then closes disk B, reopen the original disk, and eventually return to normal process.


submitted files:
	fs.c
	fs.h
	disk.c
	disk.h
	makefile
	README.md

Outside Source:
	no outer sources used for this HW5, implemented several suggestions in discussion slides 


