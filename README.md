This is a set of utility functions to create a hash table that works with either unsigned integer or string key, storing a void * , int, or unsigned int value.

The algorithm used is "hash buckets with chaining".  A hash table is created with some number of buckets, each of which is a linked list head, and as new entries are added to the hashtable, the key is hashed and used to select one of the buckets.  Entries are linked into the list corresponding to the bucket that their key hashes to.  The hashing functions are fixed, but number of buckets and amount of space for the entries is set at hash table creation time, up to an optional maximum limit of entries.

The number of buckets is intended to reduce the length of list that has to be serially searched when an entry is added or looked up, so the number of buckets chosen is a tradeoff between space and CPU time.  Some choices of number of buckets work much better than others with the supplied hashing functions and with the type of keys (e.g., random vs. sequential integers) being used.  Some recommended bucket counts will be provided in documentation (for example, 47 and 49 work better than 51 for random integer keys).

There is only one kind of hash table, users can use a particular table with either unsigned int keys or string keys, but MUST NOT mix the two types of keys in a single hash table.  The insertion and lookup functions specify the key type, not the hash table.

At present, memory used for the list entries and buckets is allocated using malloc() and is never freed.  (This can be changed in the future if desired.)

Each hash table is managed as a dynamic rsrc resource, created dynamically.  This allows hash table statistics to be printed using the rsrc PrintLong function, or by calling the corresponding hash table print function directly.

Entries can be added or modified:
```
   iHtIAddVal (table1, 25, (void *)value1);
   iHtISetVal (table1, 25, (void *)250);
   iHtSAddVal (table2, "key string", (void *)99);
```
Or looked up:
```
    printf ("value for key %d is %d\n", 25, (int) iHtIGetVal (table1, 25));
```
Or iterated through:
```
   htFOREACH(it,w,table1) {
        printf("key %d, value is: %d\n",  w-> ulKey, (int) w->pxValue);
   }
```
Or deleted:
```
   iHtSDelete (table2, "key string");
   iHtIDelete (table1, 25);
```

This is an adaptation of existing code, and was modified to use with FreeRTOS conventions. Here's the main part of the .h file:

```
#define htFORLOOP(walker,iterator) for(hashent_t *walker; (walker = pxHtIteratorNext (&iterator));)
#define htFOREACH(it,w,tab) htIterator_t it; vHtInitIterator(&it,tab); htFORLOOP(w,it)

typedef struct _htHashent {
	union  {
		Link_t xLinks;		// link to next/prev in this bucket
//		dlList_t xLinks;	// link to next/prev in this bucket
		struct _htHashent *pxFreelist;	// freelist link if entry is not in use
	};
	union {					// SI functions decide which to use, we don't care
		const char	*pcName;	// string key associated with this entry
		unsigned ulKey;		// integer key
	};
	union {
		void	*pxValue;	// value (being last forces struct alignment)
		unsigned ulValue;	// alternative access to reduce casting
		int 	lValue;
	};
} hashent_t;

typedef struct {
	Link_t *pxBuckets;		// The buckets -- an array of list heads
//	dlList_t *pxBuckets;		// The buckets -- an array of list heads
	hashent_t *pxFreelist; 	// freelist of allocated but not in use entries
	const char *pcTablename;	// name of this table, for logging/stats purposes
	unsigned ulBucketCount;	// size of buckets array at 'buckets'
							// if ==1, it's a serial search, hashing does nothing
	unsigned ulMaxEntries;	// max # entries allowed (absolute cap)
	unsigned ulCurEntries;	// count of current entries
	unsigned ulAllocSize:16;	// entries added if needed in blocks of this many
	unsigned xHasString:1;	// set if a string key has been added to the hash
	unsigned xHasInt:1;		// set if an integer key has been added to the hash
	unsigned _unused:14;	// RFU
} hashtab_t;

// allocate and initialize a new hash table, returns a pointer to it
hashtab_t *pxHtNewHashTable (const char *tablename, unsigned initentries,
						   unsigned maxentries, unsigned entryincrement,
						   unsigned numbuckets);

// Create an entry in the hash table.  It is an error to add an entry with
// an existing key, a zero will be returned.  Success is a non-zero return.
int iHtIAddVal (hashtab_t *table, unsigned key, void *value);
int iHtSAddVal (hashtab_t *table, const char *name, void *value);

// Add or modify an entry in the hash table.  The old value is replaced by the
// new one if the entry is found, and a new entry added to store the value if not.
// return value is the number of entries added, 0 or 1
int iHtISetVal (hashtab_t *table, unsigned key, void *value);
int iHtSSetVal (hashtab_t *table, const char *name, void *value);

// low-level routines that find list entries as hashent_t
hashent_t * pxHtIFindEntry (hashtab_t *table, unsigned key);
hashent_t * pxHtSFindEntry (hashtab_t *table, const char *name);

// Lookup routines to get just value, returns the value or NULL if not found
void *pvHtSGetVal (hashtab_t *table, const char *name);
void *pvHtIGetVal (hashtab_t *table, unsigned key);

// delete an entry from the hash table -- caller responsible for objects pointed to.
// I,S cases return number of deleted items, 0 or 1. EDelete assumes valid hashent_t.
int iHtSDelete (hashtab_t *table, const char *name);
int iHtIDelete (hashtab_t *table, unsigned key);
void vHtEDelete (hashent_t *entry);

// if compiled-in, print statistics of hash table
void vHtPrintStats (hashtab_t *table);

// {
//		htFOREACH(it,ht,hashtable) {
//			use ht->value etc.
//		}
		
void vHtInitIterator (htIterator_t *it, hashtab_t *table);
hashent_t *pxHtIteratorNext (htIterator_t *it);

#endif
```

