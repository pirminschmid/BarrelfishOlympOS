# Spawning a new domain
Spawning of a new domain (process) involves various steps. We split them into 2 layers: a high-level process/domain management service running in a service on one core ([process service][process] Demeter) and a low-level service running inside of each monitor for the loading of the binary and preparing the actual domain (spawn service).

Spawning of programs consists of various steps
- prepare capability space (CSpace)
- prepare virtual address space (VSpace)
- load ELF binary file (extended to load also function symbol names for [symbolic stack trace][stacktrace])
- dispatcher setup
- environment setup: including transfer of the state of the prepared [self-paging][selfpaging] handler

## Security note
During the spawn process, memory is mapped into the virtual address space (VSpace) of both domains, the host that is doing the setup and the freshly spawned domain. While memory access flags (such as read-only) are considered for the target VSpace from the beginning, the host needs write access even to these read-only regions to copy the ELF, at least temporarily. Thus, another good reason to keep this service part in the monitor that must be part of the trusted computing base (TCB) for several reasons.

## Spawn vs Fork
It was one requirement for this project to use a spawn mechanism and not a fork mechanism for the creation of new domains (processes). There are long lists of arguments for advantages and disadvantages of fork (Unix and descendents)<sup>1</sup> and spawn (basically all other systems). Knowing the ingenious fork mechanism better, it was interesting for us, to work on a spawn mechanism for this project. Practical work helps gaining a deeper understanding of the advantages and disadvantages associated with each mechanism.

In view of some of the limitations in our modified AOS Barrelfish system, in particular limited capabilities functionality, it may also have been easier to implement a spawn than to implement a fork. But since this entire process creation functionality is part of the OS library and not part of the kernel, principally both could be implemented.

**Footnotes**

1. "Implementing fork is basically implementing Unix"

[process]:../services/process.md
[stacktrace]:../miscellaneous/stacktrace.md
[selfpaging]:selfpaging.md
