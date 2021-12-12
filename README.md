# ec504_Project
Dropbox gives a fast and reliable way to send and receive files with compression. In this project, we are going to create a dropbox-like data storage system which could result in less storage capacity and increased productivity.

## INTRODUCTION
 - Dropbox-like storage in this project should accept the following functions:
 - Creating a local file system
 - Loading and encoding of text files
 - Retrieving back and decoding of text files
 - Listing of loaded files
 - Deleting of text files

Text compression can usually succeed by replacing a most frequently used character with a more common bit string. There are many alternative methods available for solving this problem. In our project, we chose fixed huffman coding (fixed-length code) to solve the problem. The text file can be effectively lowered by 25%, greatly reducing its overall size. In addition, compression can be run on the entire transmission by encoding as well as decoding. 

The project also includes a virtual file system interface which could create disk space (each disk is ~33MB), and the interface provides functions to save, load, delete, and truncate files inside the virtual disks. The virtual file system is managed using a bitmap and saves files in separated 4kb blocks so that all the gaps in the memory space could be filled.

## WORK BREAKDOWN
  - Zhiyuan Liu
      1. VFS design/codeing
      2. Compression/decompression design/decodeing
  - Jiawei Zhao
      1. Text User Interface design/codeing
      2. Documentation / slides

## GITHUB DIRECTORY
  ### Top level contains documantation:
    - Presentation Slides
    - Project Proposal
    - Final Report
  ### Code folder
    disk.c
    disk.h
    fs.c
    fs.h
    TrBigram.c
    TrBigram.h
    UI_Test.c
    makefile
    *Test input .txt files
    
