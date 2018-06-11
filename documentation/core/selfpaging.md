# Self-paging
The virtual memory address space (VSpace) of each domain needs to be backed by actual memory (RAM) on demand. In OS with monolithic kernels, this task of handling page fault events is typically part of the kernel. In Barrelfish, the CPU driver forwards a page fault from the MMU as an upcall to the domain (process) in user space where it is handled by our Barrelfish/AOS library. This mechanism is called self-paging: it allows user space processes to handle page faults by themselves. This idea has already been used in the former Nemesis OS.<sup>1</sup>  It offers the advantage that different processes may actually use different algorithms to handle page faults. Knowledge of the application domain (e.g. DBMS, multimedia, etc.) may allow optimized paging algorithms in comparison with generic algorithms of a monolithic kernel. Of course, mechanisms must be in place (capabilities in Barrelfish) to guarantee secure manipulation of memory resources in user space.

We implemented a simple form of this self-paging. The following considerations are pretty standard. We used only one page size (4 KiB). The virtual memory address space was split into memory regions that could be reserved during thread creation for stacks (regular and exception stack), shared memory buffers for the messaging system, and from the stdlib for heap extension (backend of ```malloc()```). As an example: allocating a large chunk of memory in the heap would make these reservations but would not actually allocate the RAM. The first read/write access to these addresses would then trigger a page fault that is handled by our self-paging exception handler. The size of each region has to be a multiple of the page size. Additionally, each region had an additional page that would not be mapped by memory later (guard page). Thus, access to these addresses (e.g. stack overflow) would trigger an exception and terminate the process<sup>2</sup>. Using these meta information, our page fault handler could back missing memory for the defined regions.

Memory requests were handled simply: Send an RPC to the memory service (Dionysos) to request a block of 4 KiB RAM, change capability type from RAM to frame, map this frame to proper address space (i.e. extend page table of the MMU configuration) to back the virtual memory page. Such an RPC needs one message if the requesting domain and Dionysos are running on the same core, but three messages (see intermon routing for capabilities) in the background if requesting domain runs on a different core. This intermon routing for messages with capabilites was part of the restrictions in the simplified AOS version of Barrelfish that does not allow sending capabilities between arbirtrary domains running on different cores; see [intermon service][intermon] (Poseidon's wake).

Of course this had an impact on handling page faults in domains running on different cores than the memory service. There are various options to reduce this problem:
- split the memory service into multiple memory services, one running on each core. They could use memory exchange between them to accomodate user domains with larger or smaller memory requirements running on various cores.
- add some caching of page sized memory blocks into the monitors, i.e. give them some form of mini memory service similar to the spawn services that are running in the monitors and not in the actual process admin service (Demeter)
- and, of course, building such a cache into the library that is handling the self-paging. Instead of requesting one 4 KiB RAM block at a time, a larger multiple (e.g. 1 MiB) could be requested and then split locally. This reduces the number of RPC messages sent, of course. On the other hand, this also increases the amoung of RAM that is wasted.

And since these options are all running in the user space (either as service or part of the OS library), multiple versions of the memory management / paging system can be implemented and used in parallel as it makes most sense for the system.

And of course, the capabilities system should be extended to allow sending capabilities directly between arbitrary domains running on different cores. However, this was out of scope during this project time.

**Footnotes**

1. Description of self-paging in the Nemesis OS: [link][selfpaging].
2. Any exception, that could not be handled, calls our [stack trace][stacktrace] implementation for additional debug information.

[selfpaging]: https://www.usenix.org/legacy/events/osdi99/full_papers/hand/hand.pdf
[stacktrace]: ../miscellaneous/stacktrace.md
[intermon]:../services/monitor.md
