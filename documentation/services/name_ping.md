# Detection of inactive services
As part of the [name service][name], an optional mechanism was implemented that allows automatic detection and deregistration of services that may have stopped working. This is implemented by additional timestamps that are stored in the nameserver database for each service.

With this option turned on, each server can send a periodic "ping" message to the name service (e.g. 1x/min). This is implemented by the deferred events infrastructure for which we have implemented various fixes to make such things work. This infrastructure allows registration of periodic events (defined as closures) that automatically get called by the scheduler/dispatcher.

A second thread on the nameserver periodically (e.g. every 2 minutes) checks whether the timestamps of the services are not outdated. Services with outdated timestamps are deregistered automatically. Of course, these time periods are completely arbitrary.

This system worked in testing as can be seen in the provided screen shots here.
- First a test service is launched from the shell. It registers itself at the name service Gaia
- Calling the name service UI Apollo lists all 12 registered services
- The test service is written to stop working at some point
- For debugging purpose, Gaia prints a message that the service was automatically deregistered because no ping was received
- Calling Apollo again at a later time shows only the remaining 11 services.

![auto_deregister1][auto_deregister1]

![auto_deregister2][auto_deregister2]

Of course, this feature is still of limited usability. Due to limited capability functionality (no revoke available for example), there is no cleanup implemented for domains in the entire system. Additionally, there is no automatic re-launch of such stopped services implemented yet. To offer a full service into this direction, this would also need "hot-swapping" of already connected clients from such a non-responsive service to a new one. This comes with quite some complexity that the present code base is not prepared to handle. Such things were out of scope for the given time frame.

Thus, based on this limited practical functionality and also late actual implementation of this feature, it was deactivated for the demonstration and also for the submission. It was left for optional activation. This also went hand in hand with our philosophy of providing a robust operating system as a key priority.

While the system worked well with the feature activated on my board, it is obvious that such additional periodic RPC calls to the name service changes the timing behavior of the entire distributed system. As experienced when working on the messaging system, already small changes can have quite an effect on the system: some small change could lead to loosing the current active time slice, which caused the domain to wait for the next one in 20 ms increasing latency occasionally a lot.

[name]:name.md
[auto_deregister1]:../images/auto_deregister_screenshot.jpg
[auto_deregister2]:../images/auto_deregister_screenshot2.jpg
