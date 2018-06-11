# Messaging
Good inter-process communication is key for any system, in particular for systems with minimal kernels and crucial services running in different domains such as micro-kernels and exo-kernels.

A library of low-level messaging routines was given to us as a starting point. This __Lightweight Message Passing (LMP)__ system uses register scraping and immediate context switch from sender to receiver thread to pass messages. Only 9 words (36 bytes) can be transferred per message. Since we used one word for our unified messaging system as header (message type and error codes), only 8 words (32 bytes) could be used as payload per message. Multiple messages are used for longer messages. The provided library allows sending capabilities from one thread/domain to another.<sup>1</sup> Sending a message involves a syscall (invoking the target capability). Thus, the observed properties:
- only usable between threads running on the same core
- rather low latency
- very low throughput
- capabilities can be sent

As a group effort during the development of the core functionality of our system, we first extended these library functions to a working messaging service between domains (processes) running on one core, and later added different messaging mechanisms to exchange messages also between domains running on different cores.

A __User-level Message Passing (UMP)__ system was built to send messages between threads running in domains on different cores. It is based on ringbuffers (one for each direction) in dedicated shared memory regions between the two domains. With respect to throughput, it is crucial to realize that data is shared through L1 cache without delay by writing to and reading from RAM. The cache coherency protocol allows much higher throughput than anticipated by RAM access latency. However, due to limitations of the capability system, capabilities cannot be sent directly over UMP. Property summary:
- usable between threads running on the same and on different cores
- low latency for threads on different cores; high latency (thread switch without preemption at the moment) on the same core
- high throughput
- capabilities cannot be sent

Throughput of LMP was improved a lot by a hybrid __Fast LMP (FLMP)__ system (built by Jonathan) that additionally provided ringbuffers to LMP for large messages.

All these different messaging implementations offered the same interface, of course. Thus, all RPC could be used independently of the actual messaging type. When distributing the services (see [Nameservice][nameservice]) also a workaround for sending capabilities via UMP was hidden behind the interface to keep all this transparent to higher level APIs.

**Footnotes**

1. Looking at the details, sending of capabilities by LMP between domains running on the same core is much more involved than quickly mentioned here. All code in user space -- including our OS library code -- never actually touches capabilities themselves. References to capabilities (capref) are used in the user space code. Since each domain has its own CSpace (organized set of all caprefs owned by the domain), the LMP send() does not actually send this capref to the target domain. The target domain receives its own capref to the same capability. However, receiving LMP messages is event triggered. It can happen (almost) anytime. The LMP library expects an empty prepared capref to be pre-loaded for potentially incoming caprefs. So, it was part of our job to organize our messaging system to have always such a capref preloaded. Additionally, reserving empty caprefs uses memory that can lead to messages itself to request more memory from the memory service (Dionysos). Receiving a RAM capability from Dionysos to expand the domain's CSpace needs an already preloaded empty capref. Without proper handling, this could lead to deadlocks. Thus, plenty of details for us to consider to make the system run smoothly. :-)

[nameservice]:../services/name.md
