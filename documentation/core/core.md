# Core system
The project started with one domain (process) on one core: init. It is launched by the first CPU driver (kernel) starting on core 0. This very first process is special because there is no service infrastructure available as for later domains. Solving bootstrapping properly was part of the problem description to make the system run properly.

In this domain, we first implemented a memory management system (later used in Dionysos) and as a next step the self-paging mechanisms. Additionally, all services such as serial I/O or launching of additional domains (Demeter) were first provided by this init domain (aka monitor/0). However, each of them was built as a library to facilitate distribution of the services to other separate domains later. After finishing all message systems, implementing the name service (that allows lazy binding of domains with existing services on demand), and launching the 2nd CPU core, the services of the core system could be moved to separate processes and also the additional services of the individual projects could be added.

Distributing the system into separate processes included a bootstrapping problem, in particular of the memory, name and process launch services. It was nice to see the full distributed system starting up and running rubustly even for more complex user tasks from the shell.

More detailed explanations are provided for the following core elements:
- [monitor][monitor]
- [memory][memory]
- [self-paging][selfpaging]
- [messaging][messaging]
- [spawning new domain][domainspawn]
- [spawning new core][corespawn]
- [windowing system for user I/O][windows]


[monitor]:../services/monitor.md
[memory]:../services/memory.md
[selfpaging]:selfpaging.md
[messaging]:messaging.md
[domainspawn]:domainspawn.md
[corespawn]:corespawn.md
[windows]:../miscellaneous/windows.md
