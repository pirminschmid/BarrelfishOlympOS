## Optimization of memory usage: late switching from fixed to actual allocator.
Analyzing the init process of freshly spawned domains, revealed an optimization opportunity to waste less memory and more importantly to reduce the stress of freshly spawned domains on the memory and messaging systems.

There are several RAM allocators available in the system. Some written/extended by ourselves during the group effort on the core system; all with specific properties and limitations usable for various purposes. After the implementation of [self-paging][selfpaging], the init code of new domains/processes was adjusted to switch from a default fixed RAM allocator to a more flexible allocator that could request RAM remotely from the memory service Dionysos. The fixed allocator was simple and could only provide a limited number of RAM blocks of exactly page size (4 KiB); otherwise it failed. Thus, we switched to the more flexible allocator early during initialization of the new domain (see `barrelfish_init_onthread()`).

When introducing the distributed system during the implementation of the [name service][name], latency for receiving RAM capabilities increased in particular for applications on core 1 when the memory service Dionysos was running on core 0. As explained there, re-routing of the capability carrying messages involded 3 message hops instead of 1. This could put some stress on the messaging system in particular during a critical phase of spawning a new process. Lots of memory is required then to establish several of the dynamic connections with existing services.

Thus following the principle of late binding, also a late switching from this simple fixed allocator to the actual RAM allocator was implemented. Basically, `ram_alloc_fixed()` was modified to make the switch to the proper remote RAM allocator (using Dionysos) automatically at a later time when it cannot be avoided: either none of the 256 pages of 4 KiB left (1 MiB in total), or request that could not be fulfilled by the simple fixed allocator due to requested size > 4 KiB.

To keep the simple allocator longer "in business", it was additionally adjusted to provide 4 KiB RAM blocks also to requests < 4 KiB that failed before and were part of the reason for early switch. All these changes saved memory (several 100 KiB per launched domain) and -- more importantly -- removed stress on the memory and messaging system shortly after launch of a domain.

It revealed again how small details may have a large influence on the performance of the entire system.

[selfpaging]:../core/selfpaging.md
[name]:name.md
