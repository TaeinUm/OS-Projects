# CPU Profiler
#### Taein Um


## Overview
The goal of this project is to design a CPU profiling tool as a kernel module that tracks the time spent on CPU for each task. The tool will utilize Kretprobe to monitor the `pick_next_task_fair` function of the Completely Fair Scheduler (CFS) and display the profiling results using the proc file system.


## Project Details
### Part 1: Monitoring Task Scheduling
#### 1. Setup procfs
- Goal: Create a proc file named perftop that displays "Hello World" when read.
- Tasks: Write a kernel module, set up procfs, and create the proc file.
- Deliverables:
    - Load perftop module
    - Execute cat /proc/perftop

#### 2. Setup Kretprobe
- Goal: Set up Kretprobe for the pick_next_task_fair function with pre-event and post-event handlers.
- Tasks: Increment counters in event handlers and display them in the proc file.
- Deliverables:
    - Load perftop module
    - Execute cat /proc/perftop twice with time gaps

#### 3. Count the Number of Context Switches
- Goal: Count the number of context switches where the scheduler picks a different task to run.
- Tasks: Implement logic to count context switches in the event handlers and display the count in the proc file.
- Deliverables:
    - Load perftop module
    - Execute cat /proc/perftop twice with time gaps


### Part 2: Print 10 Most Scheduled Tasks
- Goal: Track the time each task spends on the CPU and print the 10 most scheduled tasks.
- Tasks: Use rdtsc for time measurement, maintain a hash table and red-black tree for tracking tasks, and modify the proc file to display the top 10 tasks.
- Deliverables:
    - Load perftop module
    - Execute cat /proc/perftop twice with time gaps


## Technologies Used
- Programming Languages: `C`
- Operating System: `Linux`
- Libraries: `Kretprobe`, `procfs`, `spinlock`, `rdtsc`


## Contact Information
- Name: Taein Um
- Email: taeindev@gmail.com
- LinkedIn: https://www.linkedin.com/in/taein-um-00b14916b/
