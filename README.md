# Xinu Operating System (CSCI-P 536)

**The semester long project was focused on incrementally adding features to the XINU operating system and learning the principles of operating systems through implementation.**

**Topics covered**
- Setting up Xinu through QEMU and creating own commands
- Creating processes using system call
- Built a wrapper shell command
- Used Semaphores to handle producer-consumer problem
- Implemented a multi-producer multi-consumer circular queue with semaphores
- Implemented futures in different modes to solve producer-consumer problem
  - FUTURE_EXCLUSIVE: One producer and one consumer (1:1)
  - FUTURE_SHARED: One producer and multiple consumers (1:N)
  - FUTURE_QUEUE: multiple producer and multiple consumers (N:N)
- Created a multi-stream processing system
- Memory management
- Error handling and debugging 
- Created an Ext2 based RAM filesystem which handled read, write, open, close, create, seek, link and unlink operations
- Supported operations for multi-level directories along with extensive testing
