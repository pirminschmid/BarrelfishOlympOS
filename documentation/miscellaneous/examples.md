# Examples
Here are two examples of the low-level plumbing connection between the BSD standard library and the Barrelfish/AOS library after we had added our contributions.

## Simplified example: I/O
The user program uses ```printf()``` for printing. The lower end of the BSD stack is connected with the RPC API for serial I/O (Hermes) in case of stdout and with the filesystem (Plutos) for other file indicators to actually write the bytes with the proper service using the implemented messaging system for inter-process communication. It is noteworthy that these communication between domains may happen without calling into the kernel.

## Simplified example: memory
The user program uses ```malloc()``` for memory allocation. In the background, this may trigger an expansion of the allocated virtual memory space (see ```mmap()``` and ```sbrk()``` in Linux). In our implementation, a memory region is added to our virtual memory manager. Actual backing of this virtual memory with a memory page of physical RAM frame does not happen before actual access on this memory. Access to an unmapped address (see MMU) causes a page fault event that is forwarded via upcall to our [self-paging][selfpaging] mechanism in the Barrelfish/AOS library.

The RAM for the mapping is requested on demand from our memory service Dionysos running in a separate domain. Simplified, the capability system assures that only valid RAM regions owned by the domain can be used for this mapping. Additionally, it assures complete isolation of the memory regions, except two domains actually want to share the same RAM e.g. for messaging. Thus, despite manipulation of these security relevant resources in user space, the provided capability system of the TCB provides the needed security properties.

[selfpaging]:../core/selfpaging.md
