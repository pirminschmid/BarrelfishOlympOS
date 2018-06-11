Barrelfish OlympOS
==================

Barrelfish OS variant implementation for Pandaboard ES

![Logo][logo]

This has been a group project in [Advanced Operating Systems (AOS)][aos] in 2017 by Loris Diana, Matthias Lei, Jonathan Meier, and Pirmin Schmid. Here, I document some of my own experiences from this project. Some small code snippets are provided for illustration of specific issues. While the entire Barrelfish code is [open source][barrelfish_git], our developed code is not yet published here. Maybe in the future.


# Barrelfish
[Barrelfish][barrelfish] is a research operating system. It is motivated by trends in hardware design: rapidly growing number of cores and heterogeneous computing resources even in single devices (SoC, CPU, GPU, FPGA). Barrelfish is based on design principles that are fundamentally different to monolithic / hybrid kernel implementations of most frequently run OS today (Linux, macOS, Windows; alphabetical order) and to used micro kernel OS (L4, Mach, Minix<sup>1</sup>).

## Exokernel: CPU driver
A lightweight exo kernel (CPU driver) is run for each core. Most of the functionality covered inside of monolithic kernels is handled by code running in user space, either in specific services or inside of a Barrelfish/AOS library that is linked to all user programs. As examples, page faults and even actual scheduling of threads are handled by this library. The kernel itself is designed to have amenable properties for reasoning and verification such as: each CPU driver runs on a single core with a single kernel stack, stateless design, no blocking system calls, all interrupts disabled while running CPU driver code. A CPU driver never allocates memory once it is booted and thus cannot run out of memory. CPU drivers can be used in homogeneous and heterogeneous environments: "just" load the proper CPU driver matching the particular computing core.

## Capabilities
Manipulating the virtual address space inside of the userspace leads to a critical question: How can the system assure that one user space process cannot manipulate the address space of another process (unless desired)? Barrelfish uses a partitioned capabilities system for almost all objects in the system. Capabilities<sup>2</sup> are an old concept in computer science. They are of particular interest because they solve both, the naming problem and the access control problem at the same time.

Code in user space cannot handle capabilities directly but only references to such capabilities (capref) that are unique to the capability space (CSpace) of each domain (process). Basically, the trusted computing base (TCB) of Barrelfish provides an API that guarantees the security of handling caprefs and thus manipulating all resources (such as memory) from user space.

## Our tasks using Barrelfish
We worked on a slightly modified skeleton version of Barrelfish that came without several key components (such as memory management, process spawning, core spawning, message passing, file system, network, name service, shell) for which we had to implement solutions. Due to the fact that most functionality is implemented in user space (including e.g. scheduler), we did not have to modify actual kernel code (CPU driver) except for small bugfixes. We mainly worked in the OS library that gets linked to user space programs; we implemented user space services for the system such as memory, messaging, spawn, process management, etc. Code was implemented in C and little assembly. Barrelfish comes with a nice declarative build system `hake` using Haskell.

Exposure to a radically different design than the well known monolithic OS such as Linux lead to interesting insights and comparisons of these designs (advantages/disadvantages). The project gave first hand experience in solving some of the tricky problems that all OS abstract away in their provided abstraction layers.


# Pandaboard ES
We could run the system on Pandaboard ES boards with TI [OMAP4460][omap4460] SoC providing a 2 core ARMv9 CPU (32 bit). 1 GiB RAM was available. Ubuntu 16.04 LTS was used as development platform with cross-compilation by gcc for ARM and serial port connection with the running board.


# Barrelfish OlympOS
Following various milestones, we extended the initial skeleton with all the core components and services mentioned below to finally present a robust distributed system running on the board. Already early during develpment (test suite Artemis), we started naming crucial services and programs after gods and godesses of the classic Greek mythology. Thus, we added OlympOS to our Barrelfish implementation. Each service has its own [greeting message][greeting_message], of course.


Service | Namespace | Patron
--------|-----------|-------
[Monitor][monitor] | /core/monitor | Poseidon
[Inter-monitor messaging][monitor] | (privileged, in monitors) | Poseidon's wake
[Name][name] | /core/name | Gaia
[Memory (RAM)][memory] | /core/memory | Dionysos
[Process][process] | /core/process | Demeter
[Spawn][monitor] | (included in monitors) | -
[Serial][windows] | /core/serial | Hermes
Network | /core/network | Charon
Device | /core/device | Prometheus
Block device | /dev/blockdev | Electra
File system | /dev/filesystem | Plutos
Shell | /app/shell | Zeus


Program | Purpose
--------|--------
[Apollo][apollo] | User interface to name service
[Artemis][artemis] | Test suite
Zeus | Shell


## Principles
Correctness and robustness have been among our key goals for the implementations in this project. Thus, we added a testing framework in the first milestone. Additionally, we designed our modules in the group before splitting up the work and implementing them. A thorough code review for all merge requests (192 in total) has shown to be very efficient to detect problems/bugs before even merging the new code. Additionally, it helped all members of the group to become familiar with the entire code base. While certain parts of the code were refactored during the process -- mainly to have better abstraction layers -- many of the key functions (such as memory manager) kept working without bugs/changes for the entire run of the project.

## Design
The final design has one CPU driver (actual kernel, 64 KiB) running for each core. Additionally, there is a separate monitor domain running on each core. Monitors have some privileged functionality not available to other domains. And finally, service and application domains (programs) are launched on either of the cores of our dualcore board.

Services are offered by a set of remote procedure calls (RPC). A small set of such calls was provided as part of the required API, e.g. used for grading. We extended this set for the additional services that were added during the project. Additionally, we were free to map this API to any form of implementation (e.g. [message system][messaging]) as we saw fit.

Parts of the BSD standard library were provided. Thus, actual user programs can be written as on BSD systems with almost no special considerations. The linked Barrelfish/AOS library -- where we worked most -- provides the low-level plumbing to make them work. Examples of this connection between BSD standard library and Barrelfish/AOS library are shown [here][examples].

## Testing
Starting with the first milestone, we introduced a testing framework (Artemis) that simplified writing tests and running them from an automatically produced menu from the user interface (serial commandline connection). Keeping the tests with each additional milestone also allowed detection of regressions introduced with new code. Thus, problems could be detected and fixed very early.

## Core system
The core system was developed in several milestones as a group. This included the memory management system, self-paging for the virtual memory system, spawning of new domains and new cores, and the messaging system between domains on the same core and on different cores. These core services are descibed [here][core].

## Individual projects
Based on this core system, individual projects were implemented by each of us:
- Shell
- Filesystem (FAT32)
- Network (SLIP, IP, ICMP, UDP)
- [Nameservice][name]

A fifth available project would have been interesting, too: extend the provided limited capabilities system. I describe the nameservice [here][name] that was designed and implemented by myself. It also extended parts of the capabilities system to provide proper capabiltiy transfer between domains running on different cores.

## Benchmarking
For some benchmarking, the already existing [benchmarkC][benchmarkc] library could be used. More details are explained [here][benchmarking].

## Conclusion
This semester project allowed us to gain a deeper look behind the curtains of an OS. Some of the problems to solve are identical among all different types of systems (kernels), other problems were specific to this type used in Barrelfish. Within rather tight time constraints, we could present a robust system at the end that offered lots of features. Solving the problems, several key insights proved to be golden:
- Always keep some reserve around.
- Lazy evaluation / binding / etc. is often more beneficial than eager solutions.
- Use good balance of design / anticipation of problems and moving forward -- in particular in thight time constraints. A simple implementation may be all that is needed to get started. An iterative approach keeps a system working at all time.
- Yes, over-engineering is possible.<sup>3</sup>
- Good code reviews brings robustness to the system and code knowledge to all.<sup>4</sup>

## Direct links to some code samples
- [Stacktrace][stacktrace]
- [Lookahead stack allocation][lookahead]
- [Boot LEDs][boot_leds]
- [Serializable key value store][serializable_key_value_store]

## Direct link to some screen shots
- [Name service auto-ping and auto-deregistration of inactive services][name_ping]


# Source code
This is only a high-level overview of this interesting semester project. The source code may become available sometime in the future. The detailed more technical report has been submitted.

[Feedback][feedback] welcome.


# Licences
The following licenses apply:
- for my own code and this documentation: Copyright (c) 2018 Pirmin Schmid, [MIT license][license]
- for some small code snippets from Barrelfish AOS code: Copyright (c) 2007, 2008, 2009, 2010, 2011 ETH Zurich and shown under its [license][aos_license] (MIT license).


**Footnotes**

1. Minix is of particular interest again because a variant is used inside of Intel CPUs as management OS.
2. Not to be confused with capabilities as defined/used in Linux.
3. Some header files were so split up into public and private sub-header files at various locations at some point, it became hard to extend this boilerplate code correctly with new services at some time. Keep it simple is golden.
4. And I stop here before it drifts into even heavier fortune cookie style wisdom. :-)

[logo]:documentation/images/logo.png
[aos]:https://www.systems.ethz.ch/courses/fall2017/aos
[barrelfish]:http://www.barrelfish.org/
[barrelfish_git]:http://git.barrelfish.org/?p=barrelfish;a=summary
[omap4460]:http://www.ti.com/lit/ug/swpu235ab/swpu235ab.pdf
[greeting_message]:documentation/miscellaneous/greeting_message.md
[monitor]:documentation/services/monitor.md
[name]:documentation/services/name.md
[memory]:documentation/services/memory.md
[process]:documentation/services/process.md
[windows]:documentation/miscellaneous/windows.md
[apollo]:documentation/services/name.md#apollo
[artemis]:README.md#testing
[messaging]:documentation/core/messaging.md
[examples]:documentation/miscellaneous/examples.md
[core]:documentation/core/core.md
[stacktrace]:documentation/miscellaneous/stacktrace.md
[lookahead]:documentation/miscellaneous/lookahead.md
[boot_leds]:documentation/miscellaneous/boot_leds/description.md
[serializable_key_value_store]:documentation/miscellaneous/serializable_key_value_store/description.md
[benchmarkc]:https://github.com/pirminschmid/benchmarkC
[benchmarking]:documentation/miscellaneous/benchmarking.md
[name_ping]:documentation/services/name_ping.md
[feedback]:mailto:mailbox@pirmin-schmid.ch?subject=BarrelfishOlympos
[license]:LICENSE
[aos_license]:documentation/miscellaneous/Barrelfish_AOS_LICENSE
