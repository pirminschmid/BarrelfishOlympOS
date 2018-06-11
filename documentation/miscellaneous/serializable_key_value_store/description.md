# Serializable Key Value store
To fulfill the goal of providing rich meta information of the registered services, a key value store was implemented. It is based on an already provided hashtable implementation. Otherwise, already existing solutions would have been searched, or [uthash][uthash] would have been considered as basis.

Our messaging services are designed to send arbitrary binary data of arbitrary length. Thus, serialization and deserialization functions were given to the key value store. This allowed us sending entire stores as reply to queries and/or sending them with initial configuration data or as filters for advanced requests. Of course, the name service API also provided easier lookup functions based on service names without need to build such a store for a query.

Finally, functionality was added to the new store to run lambda functions over all the elements of the store. This was used for e.g. printing all elements of the store and mainly also to implement the filter. Design choices for e.g. memory ownership were made to match the provided hashtable implementation.

Source code:
- provided hashtable: [hashtable.h][ht_h], [hashtable.c][ht_c] (both with modifications)
- provided associated header files without implementation: [dictionary.h][dict_h], [multimap.h][mm_h]
- associated hakefile: [Hakefile][hakefile]
- [license][aos_license] for provided code
- new code: [serializable_key_value_store.h][store_h], [serializable_key_value_store.c][store_c]

[uthash]:https://troydhanson.github.io/uthash/
[ht_h]:include/hashtable/hashtable.h
[ht_c]:lib/hashtable/hashtable.c
[dict_h]:include/hashtable/dictionary.h
[mm_h]:include/hashtable/multimap.h
[hakefile]:lib/hashtable/Hakefile
[aos_license]:../Barrelfish_AOS_LICENSE
[store_h]:include/hashtable/serializable_key_value_store.h
[store_c]:lib/hashtable/serializable_key_value_store.c
