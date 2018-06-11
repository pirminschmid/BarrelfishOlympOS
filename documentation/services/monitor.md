# Monitor service: Poseidon
In the final distributed system, two monitor domains (monitor/0 and monitor/1) are running, one for each CPU driver. Monitors are special: each is launched by the CPU driver; monitor/0 (init) is launched at a time when no other service of the system is available. Thus, it coordinates the bootstrapping of these services and also of the additional cores (only core 1 here).

All available RAM is communicated to init during boot. As part of this bootstrapping procedure, init reserves some RAM for itself to spawn the 2nd core and initial services, but provides most of the RAM to the [memory service][memory] (Dionysos).

## Spawn service
It would be very complicated for a regular domain (e.g. process manager Demeter) to actually spawn a new domain on another core. Thus, monitors receive a message from Demeter which program needs to be launched and then the monitor associated with the specified core actually spawns the new domain, as explained [here][domainspawn].

## Intermon service: Poseidon's wake
In our system, the monitors establish privileged messaging channels among each other (intermon service, Poseidon's wake) that is used for transfer of capabilities between domains running on different cores. Due to limitations of the provided AOS Barrelfish skeleton, capabilities could not be sent directly between arbitrary domains running on different cores. Thus, a special mechanism was implemented for this as explained in more detail below. From perspective of arbitrary user domains and also of the [messaging system][messaging], this solution was entirely transparent. This extension of the messaging system was designed/built during the work on the name service.

The limitation of transferring capabilities between cores is two-fold:
- The messaging system cannot handle the transfer. Domains running on the same core use LMP as messaging channel that allows direct transfer of capabilities from domain to domain (see details [here][messaging]). Domains running on different cores must use UMP as messaging channel. These ringbuffers cannot transfer capabilities from domain to domain. However, they can transfer key information of capabilities. For capabilities describing memory regions (RAM, frame, device memory, etc.), type, base address and size are sufficient meta information.
- Available capability system does not offer such a function. It also does not provide functionality in revoking / deleting capabilities. Working on the capability system would have been a fifth available individual project. Without these extensions, it was recommended to use a kernel functionality that allows "forging" capabilities manually out of thin air. While the capability system is pretty airtight in general, this function allows monitors to generate a new capability (with associated capref) with type, base address and size information. This function is limited to monitors. Nevertheless, it's not optimal from security point of view, of course.

With these limitations, an extension to the messaging system was designed to transfer capabilities across cores. A naïve implementation would have been: (1) send the meta information to the receiver via UMP, (2) receiver requests capability from monitor using LMP. However, such an implementation would allow domains to request capabilities for arbitrary memory regions, which would break any memory isolation completely.

Therefore, the following system was implemented. The messaging system was enriched with meta information. This allowed it to automatically reroute a UMP message over monitors in case a capability was part of the message or expected as a reply of the RPC.

![rerouting][rerouting]

Regular communication across cores goes via UMP channel. In case of included or expected capability, the "UMP" message was rerouted via (1) monitor of the own core (LMP), (2) intermon communication between monitors (protected UMP channel), (3) LMP channel from monitor on destination core to target domain. The reply (potentially carrying a capability) returned on the same path. Transfer of capabilities via LMP is possible. For the transfer between monitors, the capability meta information is sent via UMP, and the receiving monitor then forges a new capability as explained. This capability is then sent to receiving domain with LMP. There are several technical details that needed to be considered, which are not discussed here.

This concept keeps at least some security guarantees. Domains could not just request arbitrary capabilities. Of course, the code of monitors must be part of the trusted code base (TCB). Despite the work needed to make this all run smoothly in the background -- there are plenty of pitfalls -- this entire procedure is completely hidden from the user of the messaging system (i.e. the RPC call). Already at this layer, the application is completely unaware of this rerouting except that it takes a bit longer.

Of course, a fully extended capability system would be much better. As mentioned, also revoking of capabilities was not possible with the given skeleton, i.e. no actual cleanup of memory is possible in the current state. But there was not enough time to run this additional project as a "side-project". Thus, the current solutions looked like the best thing possible.

BTW: it worked robustly for the system despite being used heavily in the running system. All RAM requested by domains on core 1 for self-paging came through this rerouting, all capabilities needed to establish additional communication with additional services, such as network and filesystem access, and access to the serial port for terminal I/O as well.

[memory]:memory.md
[domainspawn]:../core/domainspawn.md
[messaging]:../core/messaging.md
[rerouting]:../images/rerouting.png
