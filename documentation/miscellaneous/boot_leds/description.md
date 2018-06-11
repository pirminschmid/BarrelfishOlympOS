# Debugging problems during bootstrapping of the 2nd core
We learned a lot about the complexity involved in bootstrapping the entire system when we implemented spawning of the 2nd CPU core ([description][corespawn]). For debugging purpose (tracking down a problem that spuriously prevented the 2nd core to boot), we also extended the boot code to provide some status information by the LEDs of the board. See:
- Original [boot.S][boot_s] code (see [LICENSE][barrelfish_license])
- Modified [boot.S][modified_boot_s] (see [diff][boot_diff])
- Linked [helper C functions][helpers_c] for the LEDs

As a side effect, this added code also fixed the spurious problem.

[corespawn]:../../core/corespawn.md
[boot_s]:boot.S
[barrelfish_license]:../Barrelfish_AOS_LICENSE
[modified_boot_s]:plat_omap44xx_boot_with_leds.S
[boot_diff]:boot.diff
[helpers_c]:plat_omap44xx_boot_with_leds_helper.c
