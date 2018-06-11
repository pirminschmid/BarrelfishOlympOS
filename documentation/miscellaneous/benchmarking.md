# Benchmarking

For some benchmarking, the already existing [benchmarkC][benchmarkc] library could be used. Using floating point values in our programs actually revealed a bug in the provided kernel code. While returning from syscalls the temporarily stored floating point registers were not written back correctly from stack by the kernel.

After fixing this and some related bugs, floating point values could be used by our applications in our Barrelfish Olympos system. We submitted these patches to be embedded into the upstream Barrelfish OS.

We also provided fixes to the framework to automatically forward the needed multiplier constant (related to CPU frequency) to the application domains. Without that, time measurements have not been possible on the system.

[benchmarkc]:https://github.com/pirminschmid/benchmarkC
