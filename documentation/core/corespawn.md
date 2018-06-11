# Spawning a new core
Initially, one CPU driver (kernel) runs on core 0 of the CPU. Spawning additional cores consist of the following key steps.
- Prepare CSpace and VSpace for the CPU driver (kernel). This includes relocating its virtual addresses to the new address space.
- Prepare message channel for communication of the monitor (= init on core 0) domains running on each CPU driver. The monitors are launched by the CPU drivers.
- Transfer relevant state to the new monitor. Proper cache flushing and initialization must be used to allow this transfer. See weak sequential consistency model of ARM CPUs, inactive cache/MMU in a freshly booting core.
- Let the new core boot.

While spawning of a new application domain is based on already established services and abstraction layers, spawning of a core needs to establish these abstraction layers first, which adds to the complexity of spawning a new core. This bootstrapping problem is more pronounced for the launch of the very first CPU driver and monitor domain (init) on core 0. This leads to subtle differences in these procedures that have to be considered.

# Booting of a core
The actual boot process is quite specific for each architecture. On the high level, it involves various steps to transform the state of the core from some defined initial raw state to the fully running system. On our system, this includes in particular a switch from initially running in physical RAM space to the running in the virtual memory space as established by the defined VSpace. It was nice to see the details of the "tricks" used to perform this switch. Additionally, some state must be shared between the monitors running on each core. We solved this problem by storing only minimal information at a well-defined location. This information could be used by monitor/1 to establish a new inter-core message channel (see ringbuffer in shared memory) with monitor/0. Then, a defined RPC in our API could be used to request this state information over this intermon messaging service (Poseidon's wake).

# Debugging problems
We got a deep understanding of this entire boot process on our system that cannot be described in detail in this very high-level description here. For debugging purpose (tracking down a problem that spuriously prevented the 2nd core to boot), we also extended the boot code to provide some status information by the LEDs of the board. See:
- Original [boot.S][boot_s] code (see [LICENSE][barrelfish_license])
- Modified [boot.S][modified_boot_s] (see [diff][boot_diff])
- Linked [helper C functions][helpers_c] for the LEDs

As a side effect, this added code also fixed the spurious problem.

[boot_s]:../miscellaneous/boot_leds/boot.S
[barrelfish_license]:../miscellaneous/Barrelfish_AOS_LICENSE
[modified_boot_s]:../miscellaneous/boot_leds/plat_omap44xx_boot_with_leds.S
[boot_diff]:../miscellaneous/boot_leds/boot.diff
[helpers_c]:../miscellaneous/boot_leds/plat_omap44xx_boot_with_leds_helper.c
