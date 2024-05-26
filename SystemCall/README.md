# System Call
#### Taein Um


## Overview
The goal of this project is to learn the Linux development environment and modify Linux kernel source code. The project is divided into two parts: analyzing a user-space socket application and adding `printk()` to system call entries, and adding a new system call to encrypt a given string.


## Project Details
### Adding printk()
1. Understanding Source Code: Analyze a given user-space socket application and add comments explaining each line.
2. Adding `printk()`: Modify the kernel to print messages at the beginning of `accept()` and `connect()` system calls.
3. Testing: Test the modified kernel and capture kernel debug messages using dmesg.

### Adding a New System Call
1. Implementing the System Call: Add a new system call `sys_s2_encrypt()` that encrypts a string using a given key.
2. Writing a Test Program: Write a user-space program to test the new system call.
3. Testing: Test the system call and capture kernel debug messages using dmesg.




## Contact Information
- Name: Taein Um
- Email: taeindev@gmail.com
- LinkedIn: https://www.linkedin.com/in/taein-um-00b14916b/

