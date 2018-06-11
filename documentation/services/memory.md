# Memory service: Dionysos
Memory has been the first resource to manage in this project. It has also shown to be a quite tricky one. Already locally within the init domain in milestone 1: structs needed to manage memory need memory. And later: sending messages to request RAM capabilities needs additional memory, not only in the requesting domain and the memory service but also in the monitors that may be needed to route the capability between the two cores. And additionally, some parts of the Barrelfish AOS library need to run with deactivated dispatcher (scheduler), which deactivates the self-paging mechanism. This added an additional problem to our system that used self-paging even for the stack. Thus, plenty of opportunities and pitfalls to work on this crucial resource.

During bootstrapping, init (monitor/0) sends most of the received RAM to the memory service (Dionysos). It manages splitting large chunks of RAM into smaller ones using a [buddy allocator][buddyallocation]. This comes with limitations of internal and external fragmentation but provided a robust initial solution for RAM management. It kept working without bugs after completion in the first milestone to the end. The only change was the relocation from running within init (monitor/0) at the beginning to run in a separate service domain towards the end of the project.

Dionysos offers an RPC that allows requesting a RAM block of arbitrary size and alignment. In case of success, the capability of this RAM is returned to the requesting domain either directly if on the same core, or with rerouting as explained for the [intermon service][intermon] (Poseidon's wake). Such RAM capabilities could be retyped to frames and used for [self-paging][selfpaging], or used to build additional data structures (CSpace) while spawning new [domains][domainspawn] or [cores][corespawn] (a privilege only available to monitors).

# Some pitfalls and their solutions
## Indirect recursion of memory requests
As mentioned, memory is needed for data structures to manage memory (offered by [slab allocators][slaballocation]), to prepare slots where the fresh capability references (capref) are stored (L1CNode and L2CNodes in the CSpace), and for ARM L2 page tables while managing virtual memory (VSpace).

Thus, any request for a memory capability of arbitrary size may trigger a number of additional memory requests to prepare these data structures to fulfill the initial request. This indirect recursion was tricky to handle.

We finally came up with the concept of establishing invariants for the memory manager:
- there must always be a certain number of free slots and slabs available, such that two arbitrary requests can be fulfilled
- there must always be a certain number of blocks of certain sizes available to serve recursive requests of the slab and slot allocator, that do not need further slabs/slots

These rules defined specific numbers of RAM blocks of defined size (4 KiB and 16 KiB) that were assured to be around after each regular memory request. This assured recursive calls to be fulfilled without need to spit again blocks, which would trigger additional memory requests.

While keeping this kind of reserve around seemed to be trivial at the beginning and was part of some discussions, it worked very well and robustly for the entire run of the project.

## Messages of the RPC to request memory
Sending messages needs memory to build buffers and envelopes to send the messages around. While many messages of our API have fixed payload sizes, some RPC have to handle payload of arbitrary size or more tricky need to be handle to handle reply messages of arbitrary payload size, which may trigger memory request RPC calls while handling the reply of another message. This is fully OK for most RPCs, except for situations when such additional memory requests would be triggered while a memory request is already being handled, i.e. lack of memory exists.

Handling all these things in detail in the messaging system needed some adjustments. Among various things, it was assured that no additional memory request was triggered during critical phases of messaging, in particular not while memory requests were being handled. Additionally, solution mentioned below was also used for the messsaging system.

## Spurious exceptions of the system
We used [self-paging][selfpaging] for dynamic backing of the thread stacks. This worked very well, except for spurious crashes that happened when e.g. adding event handlers to the dispatcher. We quickly identified the problem and solved it with "look-ahead stack allocation" (see [description and code][look_ahead]. This worked so well there, that we used the same principle also to assure large enough stack sizes before critical phases of the messaging system (see above).

## Message stress during initialization of freshly spawned domains
Re-analyzing the init process of freshly spawned domains in detail during the implementation of the name service revealed an important optimization opportunity to waste less memory and more importantly to reduce the stress of freshly spawned domains on the memory and messaging systems. It showed to be very helpful in spawning new domains. It follows the principle of late/lazy action that showed to be helpful various times. Details are described [here][memory_late_switching].

[buddyallocation]:https://en.wikipedia.org/wiki/Buddy_memory_allocation
[slaballocation]:https://en.wikipedia.org/wiki/Slab_allocation
[intermon]:monitor.md
[selfpaging]:../core/selfpaging.md
[domainspawn]:../core/domainspawn.md
[corespawn]:../core/corespawn.md
[memory_late_switching]:memory_late_switching.md
[look_ahead]:../miscellaneous/lookahead.md
