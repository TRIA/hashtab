This is a set of utility functions to create a hash table that works with either unsigned integer or string key, storing a void * , int, or unsigned int value.

The algorithm used is "hash buckets with chaining".  A hash table is created with some number of buckets, each of which is a linked list head, and as new entries are added to the hashtable, the key is hashed and used to select one of the buckets.  Entries are linked into the list corresponding to the bucket that their key hashes to.  The hashing functions are fixed, but number of buckets and amount of space for the entries is set at hash table creation time, up to an optional maximum limit of entries.

The number of buckets is intended to reduce the length of list that has to be serially searched when an entry is added or looked up, so the number of buckets chosen is a tradeoff between space and CPU time.  Some choices of number of buckets work much better than others with the supplied hashing functions and with the type of keys (e.g., random vs. sequential integers) being used.  Some recommended bucket counts will be provided in documentation (for example, 47 and 49 work better than 51 for random integer keys).

There is only one kind of hash table, users can use a particular table with either unsigned int keys or string keys, but MUST NOT mix the two types of keys in a single hash table.  The insertion and lookup functions specify the key type, not the hash table.

At present, memory used for the list entries and buckets is allocated using malloc() and is never freed.  (This can be changed in the future if desired.)

Each hash table is managed as a dynamic rsrc resource, created dynamically.  This allows hash table statistics to be printed using the rsrc PrintLong function, or by calling the corresponding hash table print function directly.

Entries can be added or modified:
```
   htIAddVal (table1, 25, (void *)value1);
   htISetVal (table1, 25, (void *)250);
   htSAddVal (table2, "key string", (void *)99);
```
Or looked up:
```
    printf ("value for key %d is %d\n", 25, (int) htIGetVal (table1, 25));
```
Or iterated through:
```
   htFOREACH(it,w,table1) {
        printf("key %d, value is: %d\n",  w-> key, (int) w->value);
   }
```
Or deleted:
```
   htSDelete (table2, "key string");
   htIDelete (table1, 25);
```

This is an adaptation of existing code, and is being modified to use with FreeRTOS.  Prior to any name changes, here's the main part of the .h file:

```
#define htFORLOOP(walker,iterator) for(hashent_t *walker; (walker = htIteratorNext (&iterator));)
#define htFOREACH(it,w,tab) htIterator_t it; htInitIterator(&it,tab); htFORLOOP(w,it)

// allocate and initialize a new hash table, returns a pointer to it
hashtab_t *htNewHashTable (const char *tablename, unsigned initentries,
						   unsigned maxentries, unsigned entryincrement,
						   unsigned numbuckets);

// Create an entry in the hash table.  It is an error to add an entry with
// an existing key, a zero will be returned.  Success is a non-zero return.
int htIAddVal (hashtab_t *table, unsigned key, void *value);
int htSAddVal (hashtab_t *table, const char *name, void *value);

// Add or modify an entry in the hash table.  The old value is replaced by the
// new one if the entry is found, and a new entry added to store the value if not.
// return value is the number of entries added, 0 or 1
int htISetVal (hashtab_t *table, unsigned key, void *value);
int htSSetVal (hashtab_t *table, const char *name, void *value);

// low-level routines that find list entries as hashent_t
hashent_t * htIFindEntry (hashtab_t *table, unsigned key);
hashent_t * htSFindEntry (hashtab_t *table, const char *name);

// Lookup routines to get just value, returns the value or NULL if not found
void *htSGetVal (hashtab_t *table, const char *name);
void *htIGetVal (hashtab_t *table, unsigned key);

// delete an entry from the hash table -- caller responsible for objects pointed to.
// I,S cases return number of deleted items, 0 or 1. EDelete assumes valid hashent_t.
int htSDelete (hashtab_t *table, const char *name);
int htIDelete (hashtab_t *table, unsigned key);
void htEDelete (hashent_t *entry);

// if compiled-in, print statistics of hash table
void htPrintStats (hashtab_t *table);

```

