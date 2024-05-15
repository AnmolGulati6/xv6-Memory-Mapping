# xv6 Memory Mapping Project

## Overview
This project aims to implement memory mapping functionalities in the xv6 operating system. Memory mapping allows for the efficient management of memory by mapping regions of a process's virtual address space to physical memory. The project involves implementing system calls such as wmap, wunmap, and wremap, along with supporting system calls for debugging and testing purposes. Additionally, modifications to the fork and exit system calls are made to ensure proper handling of memory mappings during process creation and termination.

## Objectives
- Implement wmap, wunmap, and wremap system calls, which respectively allocate, deallocate, and resize memory mappings within a process's address space.
- Understand the xv6 memory layout and the relation between virtual and physical addresses.
- Implement debugging and testing system calls: getwmapinfo and getpgdirinfo, to retrieve information about memory mappings and process address space.
- Modify fork and exit system calls to handle memory mappings inheritance and cleanup.

## Project Details
Please refer to the project description for detailed information on each aspect of the implementation, including system call signatures, memory mapping modes, handling file-backed mappings, lazy allocation, and best practices.

## Getting Started
1. Clone the xv6 repository.
2. Set up your development environment for xv6.
3. Implement the required functionalities as per the project description.
4. Test your implementation thoroughly to ensure correctness and robustness.

## Usage
1. Compile and run xv6 with your implemented functionalities.
2. Test various scenarios to ensure proper behavior of memory mapping operations.
3. Utilize debugging and testing system calls for verification and validation of your implementation
