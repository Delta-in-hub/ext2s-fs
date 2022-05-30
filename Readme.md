# File System

- [File System](#file-system)
  - [Intro](#intro)
    - [FAT : File Allocation Table](#fat--file-allocation-table)
  - [Command](#command)
  - [EXT2](#ext2)

## Intro

Block array -> Block set -> file -> directory -> file system

1. Block
   1. `balloc()` and `bfree()`
      - Using list or bitmap to manage blocks
   2. `bread()` and `bwrite()`
2. File
3. Directory

### FAT : File Allocation Table

1980s, FAT was introduced. It is just for small files.
Free list is used to manage blocks.

```c++
// {[data|next],[data,next],.......}
struct block{
    char data[510];
    uint16 next;
}; 

// or 

// {next[0],next[1],.....}  .....  {data[0],data[1],........}
// This is the method that FAT uses.

```

FAT-12/16/32 (FAT entry, which is size of next pointer in bits)



## Command

- 多用户
  - 登陆
  - 注册
  - logout
- 文件系统操作
  - `open` `create` `read` `write`  `close`
  - `delete` `mkdir` `chdir` `dir` 
  - `format`





## EXT2
> https://wiki.osdev.org/Ext2#Inode_Type_and_Permissions
The root directory is Inode 2.

> https://www.vidarholen.net/contents/junk/inodes.html
> 

Note that although both the superblock and the group descriptors are duplicated in each block group, only the superblock and the group descriptors included in block group 0 are used by the kernel, while the remaining superblocks and group descriptors are left unchanged. 



`get_inode`

`get_block`

`get_entry`
