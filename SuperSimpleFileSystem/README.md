# Super Simple File System
#### Taein Um


## Overview
The goal of this project is to implement a simple pseudo file system named s2fs. A pseudo file system is not backed by disk but resides in memory and is usually used to provide information regarding the kernel to the user.


## Project Details
### 1. Create a Pseudo File System
Design a kernel module called s2fs, a mountable pseudo file system.

#### Tasks:
- Define struct `file_system_type s2fs_type`.
- Use `mount_nodev` to mount a pseudo file system.
- Define a function named `int s2fs_fill_super(...)` to fill a superblock and pass it as an argument for `mount_nodev`.
- Register and unregister the s2fs_type filesystem during module init and exit.

#### Deliverables:
- Run the following commands:
```sh
$ mkdir mnt
$ sudo insmod s2fs.ko # Inserting the filesystem kernel module
$ sudo mount -t s2fs nodev mnt # Mount at dir mnt with option nodev
$ mount # Print out all mounted file systems
$ sudo umount ./mnt # Unmount the file system
$ sudo rmmod s2fs.ko # Remove the kernel module
```

### 2. Implement File Operations
1. **Create an Inode**: Write a function to create an inode.
2. **Create a Directory**: Write a function to create a directory.
3. **File Operations**: Implement functions to handle file operations.
4. **Create a File**: Write a function to create a file.
5. **Putting it All Together**: Update `s2fs_fill_super()` function to create a directory named foo and a file named bar inside it.

#### Deliverables:
Run the following commands:
```sh
$ sudo insmod s2fs.ko
$ sudo mount -t s2fs nodev mnt # mount the filesystem
$ cd mnt/foo # change the directory
$ cat bar # read bar. check if you can see ``Hello World!''
$ cd ../..
$ sudo umount ./mnt # unmount
$ sudo rmmod s2fs.ko
```


## Technologies Used
- Programming Languages: `C`
- Operating System: `Linux`
- Tools: `GCC`, `Make`


## Contact Information
- Name: Taein Um
- Email: taeindev@gmail.com
- LinkedIn: https://www.linkedin.com/in/taein-um-00b14916b/
