# Process service: Demeter
Launching a service or application from the multiboot image or SD card is done in the final system with RPC calls to Demeter. This process service keeps track of all running processes. For the actual spawn of the programs, it sends an RPC to the spawn service of [monitor][monitor] on core 0 or 1 dependent on which core the new application shall run.

# Spawn service: included in monitor
Spawning of programs consists of various steps of the core system described [here][domainspawn].

[monitor]:monitor.md
[domainspawn]:../core/domainspawn.md
