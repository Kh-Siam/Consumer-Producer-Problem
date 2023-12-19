# Producer and Consumer Problem Simulation

## Description
The Producer Consumer problem is a process synchronization problem. In this problem, there is a memory buffer of a fixed size. Two processes access the shared buffer: Producer and Consumer. A producer creates new items and adds to the buffer, while a consumer picks items from the shared buffer.

This is a simulation to the consumer and producer problem that takes place in the operating systems to synchronize the processes actions.

This code utilizes semaphores which are integer variables, shared among multiple processes to synchronize access for common resrources.

You can add producers and consumers as much as you want while the code is running and you'll see the results of such additions on the printed table.

> This code only works in linux environment as it prints out on the linux terminal.