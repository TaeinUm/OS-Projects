# Super Simple Distributed Shared Memory
#### Taein Um

## Overview
The goal of this project is to implement s2dsm, a distributed shared memory program in which two processes share a set of pages in a coherent manner. This project is divided into two parts: understanding userfaultfd and setting up shared memory, and implementing the MSI protocol for page coherence.




## Project Details

### Part 1: Understanding userfaultfd and Initial Setup
#### 1. Understanding userfaultfd Demo
- Goal: Understand how userfaultfd can be used to handle page faults at the user level.
- Tasks: Comment on each line of the provided `uffd.c` source code explaining its functionality and the output of the demo.

#### 2. Pairing Memory Regions Between Two Processes
- Goal: Create a user-land application named s2dsm that allows two processes to share memory regions.
- Tasks: Implement socket communication between two processes, allocate and share memory regions using mmap.

#### 3. Implement userfaultfd
- Goal: Enhance `s2dsm` to register memory regions with userfaultfd and handle page faults.
- Tasks: Implement read and write commands to handle page faults and manage memory content.

### Part 2: Implement MSI Page Coherence Protocol
- Goal: Ensure coherence of shared memory regions between two processes using the MSI protocol.
- Tasks: Implement the MSI protocol for managing memory state transitions, handle read and write operations, and view MSI array contents.


## Technologies Used
- Programming Languages: `C`
- Operating System: `Linux`
- Libraries: `POSIX Threads (pthread)`, `userfaultfd`, `Sockets`
- Tools: `GCC`, `Make`, `tmux`




## Contact Information
- Name: Taein Um
- Email: taeindev@gmail.com
- LinkedIn: https://www.linkedin.com/in/taein-um-00b14916b/
