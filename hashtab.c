/*
 *  hashtab.c
 *
 *  Copyright 2010,2022 TRIA Network Systems. See LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>
#include "hashtab.h"
#include "rsrc.h"	// hash tables are allocated from a pool

#ifndef _LISTUTILS_H_
#define dlList_t 	Link_t		// compatible libraries, different names...
#define LLINKSINIT	listINIT_HEAD
#define lInsert		listADD
#define lDelete		listREMOVE
#define right		pxNext
#define left		pxPrev
#endif

// For use in FreeRTOS, translate POSIX APIs
#define POSIX 1
#ifdef POSIX
#include <stdio.h>
#define DEBUGPRINTF(x...)	printf(x)
#define logPrintf(x...)		printf(x)

#else // for now, assume FreeRTOS
#define malloc(x)	pvRsMemAlloc(x)
#define free(res)	vRsMemFree(res)
//#define DEBUGPRINTF			LOGI(TAG,x...)
#define DEBUGPRINTF(x...)
#define logPrintf(x...)			LOGI(TAG,x...)

static const char* TAG = "hashtab";
#endif // POSIX

#define htOVERWRITE 1		// overwrite existing entry with same key?
#define htNOOVERWRITE 0
#define htPRINTSTATS	1	// if set, detailed statistics printing enabled

// allocate and add more entries to freelist, if allowed and if malloc succeeds
static int morefree(hashtab_t *tab, unsigned num2add)
{
	int i;
	hashent_t *e;
	unsigned size = sizeof(hashent_t);
	
	size += sizeof (size_t) - (size & (sizeof (size_t) - 1));
	
	// check if we're allowed to add more, and if so, can acquire space
	if ((tab->maxEntries && tab->curEntries + num2add > tab->maxEntries)
		 || !(e = (hashent_t *) malloc(size * num2add))) {
		return (0);
	}
	for (i = 0; i < num2add; i++) {
		e->freelist = tab->freelist; // put entry on free list
		tab->freelist = e;
		void *alignit = e; alignit += size; e = alignit; // nuts
	}
	return (tab->allocSize);
}

// Get a free entry from the freelist of a table to add to the table
static hashent_t *newhashent(hashtab_t *table)
{
	hashent_t *e = table->freelist;
	
	if (e) {
		table->freelist = (hashent_t *)e->freelist;
	} else {
		if (morefree(table, table->allocSize))
			return (newhashent(table));
	}
	table->curEntries++;
	LLINKSINIT((dlList_t *)e);
	return(e);
}
static void freehashent (hashtab_t *table, hashent_t *entry)
{
	entry->freelist = table->freelist;
	table->freelist = entry;
	table->curEntries--;
}

// Hashing functions.  Feel free to improve this, it's ad-hoc
static inline unsigned hashedname(const char *name)
{
	unsigned hash = 0;
	
	for (; *name; name++)
		hash = (hash >> 16) + ((hash << 5) ^ (*name));
	return (hash);
}
static inline unsigned hashedint (unsigned key)
{
	return (277 * key + key + 12345);
}

// Hash table lookup common routine.  Used to find the correct listhead, and if
// the entry is present, the correct hash entry.  Returns non-zero if the entry
// was found.  The listhead arg is where we return the list it should have been in.
// if name is NULL, the key is used to determine a match. If not, strcmp is used
static int hashLookupCom (hashtab_t *table, unsigned key, const char *name,
				   dlList_t **listheadp, hashent_t **entry)
{
	dlList_t *listhead;		// correct list for this name
	hashent_t *e;			// the roamer through the list off the head
	int bucketno = (name ? hashedname (name) : hashedint(key) ) % table->bucketCount;
	
	*listheadp = listhead = &table->buckets[bucketno];
	
	e = (hashent_t *)listhead->right;
	for (; (dlList_t *)e != listhead; e = (hashent_t *)e->links.right) {
		if (!name && key != e->key)
			continue;
		if (name && strcmp(name, e->name) != 0)
			continue;
		*entry = e;
		return (1);
	}
	*entry = NULL;
	return (0);
}
hashent_t * htIFindEntry (hashtab_t *table, unsigned key)
{
	dlList_t *listhead;
	hashent_t *e;
	
	if (hashLookupCom(table, key, NULL, &listhead, &e))
		return (e);
	return (NULL);
}
hashent_t * htSFindEntry (hashtab_t *table, const char *name)
{
	dlList_t *listhead;
	hashent_t *e;
	
	if (hashLookupCom(table, 0, name, &listhead, &e))
		return (e);
	return (NULL);
}

// Hash lookup general lookup routines.  Pass a name, get back a value, or not.
void *htSGetVal (hashtab_t *table, const char *name)
{
	dlList_t *listhead;		// dumping ground - don't need this
	hashent_t *e;
	
	(void) hashLookupCom(table, 0, name, &listhead, &e);
	return (e ? e->value : NULL);
}
void *htIGetVal (hashtab_t *table, unsigned key)
{
	dlList_t *listhead;		// dumping ground - don't need this
	hashent_t *e;
	
	(void) hashLookupCom(table, key, NULL, &listhead, &e);
	return (e ? e->value : NULL);
}

// Finds an entry, selectively rewrites its value if found, adds it if not
static int htISAddVal (hashtab_t *table, int overwrite, unsigned key, const char *name, void *value)
{
	dlList_t *listhead;
	hashent_t *e;
	
	if (hashLookupCom(table, key, name, &listhead, &e)) {
		if (overwrite == htOVERWRITE) {
			e->value = value;
			return (1);
		}
		else
			return (0);					//   already there, we don't touch it
	}
	e = newhashent(table);				// new entry, create and fill it
	if (name)
		e->name = name;
	else
		e->key = key;
	e->value = value;
//	DEBUGPRINTF("entry %p, head %p (%p, %p): ", e, listhead, listhead->pxNext, listhead->pxPrev);
	lInsert(listhead, (dlList_t *) e);
//	DEBUGPRINTF("now: entry (%p, %p), head (%p, %p)\n", ((dlList_t *)e)->pxNext, ((dlList_t *)e)->pxPrev,listhead->pxNext, listhead->pxPrev);
	return (1);
}
int htIAddVal (hashtab_t *table, unsigned key, void *value)
{
	return htISAddVal (table, htNOOVERWRITE, key, NULL, value);
}
int htSAddVal (hashtab_t *table, const char *name, void *value)
{
	return htISAddVal(table, htNOOVERWRITE, 0, name, value);
}
int htISetVal (hashtab_t *table, unsigned key, void *value)
{
	return htISAddVal(table, htOVERWRITE, key, NULL, value);
}
int htSSetVal (hashtab_t *table, const char *name, void *value)
{
	return htISAddVal(table, htOVERWRITE, 0, name, value);
}

// delete an entry from the hash table -- frees storage, entry
// returns number of deleted items, 0 or 1.  
// if keep == HT_FREEOLDVALUE, we free() the deleted entry's value.
int htISDelete (hashtab_t *table, unsigned key, const char *name)
{
	dlList_t *listhead;
	hashent_t *e;

	if (hashLookupCom(table, key, name, &listhead, &e)) {
		lDelete ((dlList_t *) e);// unlink it
		freehashent (table, e);			// put entry on free list
		return (1);
	}
	return (0);
}
int htSDelete (hashtab_t *table, const char *name)
{
	return (htISDelete (table, 0, name));
}
int htIDelete (hashtab_t *table, unsigned key)
{
	return (htISDelete (table, key, NULL));
}
void htEDelete (hashent_t *entry)
{
	lDelete ((dlList_t *)entry);
}

static void nextentry (htIterator_t *it) {
	// step through the buckets, and for each, step through the chain
	while (it->bucket < it->table->bucketCount) {
		hashent_t *curbucket = (hashent_t *)&it->table->buckets[it->bucket];
		
		if((it->next = (hashent_t *)((dlList_t *)it->next)->right) != curbucket) {
			return;
		}
		if (++it->bucket < it->table->bucketCount)
			it->next = (hashent_t *)&it->table->buckets[it->bucket];
	}
	it->next = NULL;
}
void htInitIterator (htIterator_t *it, hashtab_t *table)
{
	it->table = table;
	it->bucket = 0;
	it->next = (hashent_t *)&table->buckets[0];
	// find the next/first entry, if there are any
	nextentry(it);
}
hashent_t *htIteratorNext (htIterator_t *it)
{
	hashent_t *retval = it->next;
	
	nextentry(it);
	return (retval);
}

// **************************************************
// We are making hash tables into a rsrc resource.  We can in the future,
// once we keep track of hash table mallocs, make it possible to free them
// up.  For now, we create a pool of hash tables -- once -- and then allocate
// new ones from that pool when they are created.  Hash entry blocks could also
// be allocated using rsrc, which might make sense, but we don't for now.
// ***************************************************

static rsrcPoolP_t hashTablePool;	// allocated once, when first needed

// The "long" print routine for hash table resources calls this
static void printStatWrapper (rsrcPoolP_t pool, void *body)
{
	htPrintStats ((hashtab_t *)body);
}
void static inline initHashtabPool ()
{
	if (hashTablePool)
		return;
	hashTablePool = pxRsrcNewDynPool("HashTables", sizeof (hashtab_t), 0);
	if (!hashTablePool) {
		logPrintf("Can't allocate HASHTables rsrc pool, aborting now\n");
		abort();
	}
	vRsrcSetPrintHelper(hashTablePool, printStatWrapper);
}

// Allocate and initialize a hash table
hashtab_t *pxHtNewHashTable (const char *tablename, unsigned initentries, unsigned maxentries, unsigned entryincrement, unsigned numbuckets)
{
	hashtab_t *tab;
	dlList_t *listheads;
	int i;
	
	initHashtabPool();
	
	if (entryincrement > htMAXALLOCSIZE) {
		entryincrement = htMAXALLOCSIZE;
	}
	tab = pxRsrcAlloc(hashTablePool, tablename);
	numbuckets |= 1;	// avoid degenerate case of even bucket count
	if (tab == NULL
		|| (listheads = (dlList_t *)malloc(sizeof (dlList_t) * numbuckets)) == NULL) {
		DEBUGPRINTF("unable to allocate buckets/entries for hashtable\n");
		if (tab)
			vRsrcFree(tab);	// safe to delete, no storage will be lost
		return (NULL);
	}
	for (i = 0; i < numbuckets; i++) {
		LLINKSINIT(&listheads[i]);
	}
	tab->tablename = tablename;
	tab->maxEntries = maxentries;
	tab->curEntries = 0;
	tab->allocSize = entryincrement;
	tab->bucketCount = numbuckets;
	tab->buckets = listheads;
	tab->freelist = NULL;
	morefree(tab, initentries);
	return (tab);
}

#ifdef htPRINTSTATS
#define MAXCHAINLEN 32
static int listlength (dlList_t *list)
{
	int len = 0;
	
	// loop through list, add one for each member
	hashent_t *e;			// the roamer through the list off the head
	
	e = (hashent_t *)list->right;
	for (; (dlList_t *)e != list; e = (hashent_t *)e->links.right) {
		len++;
	}
	return (len);
}
void htPrintStats(hashtab_t *table)
{
	int chainlengths[MAXCHAINLEN]; // number chains with each length
	int overmax = 0;		// length over the most we're istogramming
	float idealchainlen = (float) table->curEntries / (float) table->bucketCount;
	int longest = 0;
	
	memset(chainlengths, 0, sizeof chainlengths);
	// loop through buckets, create histogram of chain lengths
	// The ideal is that chain actual lengths should cluster closely around
	// the ideal -- which is the number of entries divided by nmber of buckets
	for (int i = 0; i < table->bucketCount; i++) {
		int len = listlength(&table->buckets[i]);

		if (len > longest)
			longest = len;
		if (len >= MAXCHAINLEN) {
			overmax++;
		} else {
			chainlengths[len]++;
		}
	}
	
	// summarize findings
	logPrintf("\nTABLE \"%s\"\n", table->tablename);
	logPrintf("BUCKETS: %d, MAX_ENTRIES %d, CUR_ENTRIES %d, INCREMENT %d\n", table->bucketCount, table->maxEntries, table->curEntries, table->allocSize);
	logPrintf("CHAIN  CHAIN\nLENGTH COUNT\n");
	for (int i = 0; i < MAXCHAINLEN; i++) {
		if (chainlengths[i]) {
			logPrintf("%6d: %d\n", i, chainlengths[i]);
		}
	}
	logPrintf("Ideal average chain length: %7.2f\n", idealchainlen);
	logPrintf("Longest chain %d\n", longest);
	logPrintf("CHAINS OVER %d: %d\n", MAXCHAINLEN, overmax);
	logPrintf("EMPTY BUCKETS: %d\n", chainlengths[0]);
}
#else
void htPrintStats(hashtab_t *table)
{
}
#endif
