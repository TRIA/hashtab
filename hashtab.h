/*
 *  hashtab.h
 *
 *  Copyright 2010,2022 TRIA Network Systems. See LICENSE file for details.
 */

#ifndef _HASHTAB_H_
#define _HASHTAB_H_

#include "iot_doubly_linked_list.h"
// #define dlList_t Link_t
// #include "listutils.h"	// compatible calling sequence, different names

#define LL_LOG_HASHTAB		"hashtab"
#define htMAXALLOCSIZE		0xffff	// max that'll fit in the field in hashtab_t

#define htFORLOOP(walker,iterator) for(hashent_t *walker; (walker = htIteratorNext (&iterator));)
#define htFOREACH(it,w,tab) htIterator_t it; htInitIterator(&it,tab); htFORLOOP(w,it)


typedef struct _htHashent {
	union  {
		Link_t links;		// link to next/prev in this bucket
//		dlList_t links;		// link to next/prev in this bucket
		struct _htHashent *freelist;	// freelist link if entry is not in use
	};
	union {					// SI functions decide which to use, we don't care
		const char	*name;	// string key associated with this entry
		unsigned key;		// integer key
	};
	union {
		void	*value;		// value (being last forces struct alignment)
		unsigned uintvalue;	// alternative access to reduce casting
		int intvalue;
	};
} hashent_t;

typedef struct {
	Link_t *buckets;		// The buckets -- an array of list heads
//	dlList_t *buckets;		// The buckets -- an array of list heads
	hashent_t *freelist; 	// freelist of allocated but not in use entries
	const char *tablename;	// name of this table, for logging/stats purposes
	unsigned bucketCount;	// size of buckets array at 'buckets'
							// if ==1, it's a serial search, hashing does nothing
	unsigned maxEntries;	// max # entries allowed (absolute cap)
	unsigned curEntries;	// count of current entries
	unsigned allocSize:16;	// entries added if needed in blocks of this many
	unsigned flag1:1;		// RFU (was: case independent strings)
	unsigned flag2:1;		// RFU (was: htAdd* generates error)
	unsigned unused:14;		// RFU
} hashtab_t;

// allocate and initialize a new hash table, returns a pointer to it
hashtab_t *pxHtNewHashTable (const char *tablename, unsigned initentries,
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

// iterator and initialization for stepping through a hash.
typedef struct  {
	hashtab_t	*table;
	hashent_t	*next;	// next to be returned on call to htIteratorNext
	unsigned	bucket;	// index of bucket that holds *next
} htIterator_t;

// Iterator.  Note that since hash tables are sparse, a function call is needed to find next.
// Since NULL is a valid value, the iterator operates on hash entries, so callers need to
// extract the value (or string) themselves.  Note that it is safe to delete the entry being
// examined, as the next entry has already been selected.  However, if the table can be
// simultaneously modified by multiple threads, mutual exclusion must be used outside these
// functions/macros.  If a new entty is added during walking the list,
// it is undefined whether that entry will be subsequntly visited or not.
// Used as follows:
// {
// 		htIterator_t it; htInitIterator (&it, hashtable);
//
//		for (hashent_t *ht; (ht = htIteratorNext (&it));) {
//			use ht->value etc.
//		}
//
// or:
// {
//		htFOREACH(it,ht,hashtable) {
//			use ht->value etc.
//		}
		
void htInitIterator (htIterator_t *it, hashtab_t *table);
hashent_t *htIteratorNext (htIterator_t *it);

#endif
