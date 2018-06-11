# Windowing system for serial I/O
When we extended the system to allow multiple domains (processes) to run in parallel, the question arose how to handle the output of multiple processes in parallel and also how to determine which of the applications would actually get the user input that is provided over the serial interface.

We used a simple but effective method to provide control to the user. The serial I/O system (Hermes) was designed to use separate input and output buffers for each domain (process). Only one process at the time is visible. These outputs are forwarded directly to the serial interface, inputs are stored in the input buffer associated in case the application is not reading as many characters as provided.

The buffers were designed to keep at least a full screen size. Thus, upen switch of the visible domain, information of the new domain and also potentially unseen content could be transmitted over the serial interface.

Of course, this simple system cannot be compared to graphical user interfaces of other OS. But it provided the functionality to avoid mixed up outputs and inputs.
