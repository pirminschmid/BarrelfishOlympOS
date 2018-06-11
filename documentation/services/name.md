# Name service: Gaia
## Limitations of the core system
At the time when we all had to focus on our individual projects, our core system robustly provided the needed core features as defined (see [description][core]). However, there were some limitations to the system, some interconnected to each other.
- Messages could be sent from domains to domains running on the same and also different cores ([messaging system][messaging]), but capabilities could not yet be sent across cores in general. As a workaround, available memory was split in half and 2 separate memory services were running inside of the monitors on each core. All other messages did not need capabilities at that time (see serial I/O mainly).
- Most services were provided by init (monitor/0) to all launched domains. Of course, we had a distributed system in mind from the beginning. But this monolithic service provider was easier to implement with the given limitations of the core system.
- Lacking a service that allowed looking up other services and making connections dynamically, the capabilities for the message channels for each service had to be provided statically in the spawn structure during the spawn. This worked well for the few services of the core system, but would not scale to more and more services to come.

## Goals of my individual project
The official goals of my individual project were:
- define a name space for the services including rich meta information of the services
- design and implement a name service that allows dynamic late lookup and binding of services with other domains
- design and implement a user interface to access the information stored in the name service.
- optional: provide auto-detection whether a service is no longer running

Given the limitations of the core system, these additional tasks needed to be accomplished to make this work.
- provide an extension to the existing messaging system to allow sending capabilities safely across cores; best leaving the current messaging API untouched and let the extension work completely transparently in the background
- distribute the services into different domains (all were still provided by monitor/0 or monitor/1 at that time)
- define minimum number of services that needed to be given to new domains at the "well known locations" in the spawn structure, while all the others could be linked dynamically
- solve the bootstrapping problem that came with this

It was my personal goal to provide the core functionality of this name service (registration, lookup and binding) as early as possible to my colleages so they could start using it when they aimed at testing the integration of their implementations of network and filesystem. Otherwise, temporary workaround solutions would have been built first.<sup>1</sup>

## Namespace
A hierarchical namespace was established for all services. A service name must adhere to the following rules:
- Scheme: `/<class>/<type>[/<subtype>]/<enumeration>`
- a service name is the full absolute path starting with `/`
- subtype is optional and not used at the moment for the small current system
- the characters of all `<class>`, `<type>`, `<subtype>` parts must be in {'a'..'z', '0'..'9', '_'}
- `<enumeration>` is a string representation of a valid size_t value and is assigned by the name server upon registration

Additionally, a short name can be used that conforms to the rule `<prefix><enumeration>`. `<enumeration>` is identical to the full name. `<prefix>` can be considered a shortcut for `/<class>/<type>/<subtype>/`. Characters of `<prefix>` must be in {’a’..’z’}
And finally, services are registered with rich meta data information, that is stored in a serializable key value store and that can also be used for lookup. Keys must be in {’a’..’z’, 0’..’9’, ’_’} with reserved keywords such as class, type, subtype, enumeration, name, short name, which are added during registration. Values are strings.

## Nameserver
The name service provides data collection about all available services of the system, and most importantly plays the role of a "chaperon" when a domain (user program or service) needs a connection with an already existing service.

### Registration and lookup
Services register themselves at the name service providing key information (see figure): a name in the namespace (without the enumeration), interface/service type (corresponding to the function group FG), core_id, bandwidth information. These and additional meta data are stored in the name service. The registering service receives its own handle and a specific registration key (basically a hashsum) upon success. This registration key is used for additional functions for the service later like deregister, ping and update. It prevents arbitrary other domains to modify the service data.

![register_and_lookup][register_and_lookup]

The enumeration provided by the name service guarantees a unique name for each service. Registration is very easy for a service. A call to a single library function (`aos_rpc_service_register()`) performs various registrations in the system:
- registers the proper event handler in the service to be registered
- registers the service to the name service via `aos_rpc_name_service_register()` RPC call to name service
- registers the service with its monitor for intermon services; this simplifies intermon service resolution later
- optionally registers periodic auto-ping event in the name service to detect when a service is down

Various lookup methods allow clients to find services, such as full name or short name (prefix) of the service, or filtering based on provided meta information in the key value store. Lookups return handles to services that can be used for bindings or retrieving detailed information (actually a copy of the registered key value store).

### Late binding of services
The service handle, received by a lookup operation, can be used to establish a connection with the desired service. For this, a bind operation is called on the name service (see figure), which allows all domains to perform late binding with any service.

![late_binding][late_binding]

The name service plays the role of a chaperon. Based on information in the bind request (3a; e.g. core_id of the requesting client) and stored information in its database (e.g. high bandwidth), the name service requests a matching channel endpoint (LMP, FLMP, or UMP) at the actual service while handling the bind request (3b and 3c). This endpoint is then forwarded to the client (3d) that can establish an actual communication channel with the service (4). Also routing information is provided to the client. This routing information consists of 2 bytes (note: coreid_t is limited to one byte) that basically holds the sender and target core_ids. If they are not equal, the re-routing system can later easily determine whether re-routing is needed. No re-routing is done of course if both core_ids match.

For improved convenience of the client program code, there is a wrapper for the actual lowlevel bind operation that makes the actual server connections. The current system provides a very convenient high level function
(`aos_rpc_get_channel()`) that is used in all regular RPC channel establishing functions. These late binding functions are also used to create socket connections with the network service.

Despite the convenience, several communications need to go correctly in the background to establish proper connections, of course. It works well and robustly so far.

### Rich collection of meta information
The name service was built to offer rich meta data for each service. For this purpose, a serializable key-value store was implemented (see [implementation][store]). It bases on the provided hashtable implementation but offers various improved features, most notably a built-in serialization / deserialization method that comes very handy for all the message payload transfer in the RPC calls. Additionally, it offers an iterator for a lambda function. Thanks to this improved container, the nameserver implementation could be kept simpler and also the nameserver client UI program (Apollo) could be kept short.

### Detection of inactive services
A mechanism was implemented that allows detection and automatic deregistration of services that may have stopped working. This example also shows some screen shots of the running system. See [ping service][ping].


## Extension to the messaging system
The message system must be able to send capabilities across different cores to allow late binding of applications and services running on different cores. Thus, the messaging system was extended to handle re-routing of messages over the monitors as explained for the [intermon service][intermon] service provided by the monitors.

## Distribution of the services
With all these mechanisms implemented, the services could be moved from monitors to separate domains that were launched, then registered themselves with the name service and were connected only when needed by other services or application programs following the concept of late binding.

The number of provided messaging capabilites during spawn was reduced to 2. Each freshly spawned domain received a messaging endpoint for its monitor (either monitor/0 or monitor/1) and an endpoint for the name service. All other services are connected with late binding.<sup>2</sup>

## Service bootstrapping
As soon as all key services are available –- i.e. both monitors, intermon, memory, name and process services -– all new domains can be launched via regular process service RPC call (Demeter). Before this stable situation some specific precautions must be made during the bootstrapping. Gaia e.g. is launched without memory service. It is added later when Dionysos actually registers.

Thus, the entire bootstrap process was reorganized while distributing the services from monolithic system to sepa- rate domains. Some specific RPC functions have been introduced just to facilitate this bootstrap mechanisms including some specific synchronization primitives to keep the spawning of the services in sync.
The current bootstrap process follows this sequence, which is also documented in the source code of `usr/init`.
- spawn memory service Dionysos (only receives /core/monitor/0) -> /core/memory/0
- init loads most RAM to Dionysos. Several MiB have been split away for a local memory allocator to guarantee proper bootstrapping and spawning of the 2nd core.
- spawn nameserver service Gaia (receives /core/monitor/0) -> /core/name/0; nameserver connects with /core/memory/0 when the memory service registers itself at the name service
- RPC sent to Dionysos to register himself to Gaia; Gaia uses this opportunity to connect with memory service
- spawn new monitor on core 1; receives /core/monitor/0 as intermon connection and /core/name/0 -> /core/monitor/1; establishes connection with memory service dynamically using late binding
- spawn process service Demeter -> /core/process/0
- RPC call to Demeter to manually add init, Dionysos, Gaia, monitor/1 and Demeter to the list of spawned
domains including domain ids
- the rest of the system can use regular spawn via Demeter, i.e. the other services may be launched on either core0 or core1
- serial I/O Hermes -> /core/serial/0 is the first of these "regular" spawns
- spawn of other services; each registers itself with Gaia
- Zeus is launched and offers the shell to the user


## Optimization of memory usage: late switching from fixed to actual allocator.
Re-analyzing the init process of freshly spawned domains in detail revealed an important optimization opportunity to waste less memory and -- more importantly -- to reduce the stress of freshly spawned domains on the memory and messaging systems. It showed to be very helpful in spawning new domains. It follows the principle of late/lazy action that showed to be helpful various times. Details are described [here][memory_late_switching].


## Apollo
A simple program provided a user interface to access the data stored in the name service. Search and filtering options were available for all information stored in the rich meta data in the [key value store][store]. Some screen shots of this program are shown [here][ping].


## Conclusion
Within the month of the personal projects, all these mentioned goals were achieved. The design is straight-forward. However, implementation required solving many tricky things to have it working smoothly at the end.

Although almost nothing can be seen from the name service that just works in the background, it has been a very rewarding feeling to see that the current system works robustly despite making quite heavy use of all these late binding mechanisms offered by the name service.

They are involved during bootstrap, during spawn of each domain, during connection of the filesystem, while establishing new network sockets, and while spawning new Zeus shell instances, when new additional `/app/shell` instances are registered for the domains that are launched from within the new shell.

Additionally, it has been a goal of the implementation to hide as much complexity of this system in the background. I think that this goal has been achieved: Intermon re-routing in case of capability transfers works completely transparently within the messaging system. Besides a longer latency, the re-routed RPC channel is completely unaware of this feature. Server registration (on various registration levels) basically happens with creation of a key value store and call of one registration function. And finally, the late binding mechanisms are completely hidden behind the RPC get channel functions (except during bootstrapping of course).

Together with the very nicely implemented individual projects of my colleagues we could present a robust distributed Barrelfish OlympOS system with quite rich functionality on December 22, 2017.


**Footnotes**

1. see also "There is nothing more permanent than a temporary solution." (author?).
2. The number of provided endpoints could even be reduced to 1, i.e. only the monitor. There could be an additional monitor RPC that could establish a fresh connection with the name service. However, there is no point in minimizing this and put additional messaging strains on the freshly starting domain.

[core]:../core/core.md
[messaging]:../core/messaging.md
[store]:../miscellaneous/serializable_key_value_store/description.md
[register_and_lookup]:../images/register_and_lookup.png
[late_binding]:../images/late_binding.png
[ping]:name_ping.md
[intermon]:monitor.md
[memory_late_switching]:memory_late_switching.md
