#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include "hashtab.h"
#include "rsrc.h"


/***********************************************************************************
 *
 * This function is called from an embedded ESP32 application after initializtion to
 * ensure that hashtab works correctly in that environment.  (It did when last run.)
 *
 ***********************************************************************************
 */

void printresult (int errors, const char *message)
{
	printf ("Test '%s': %s\n", message, (errors ? "FAIL" : "PASS"));
}

#define NUMINTKEYS_H1  200

void hashtest() {
	int somevalue = -1; // any of the values we put into h1
	int errors;	// used inside loops to accumulate error count, if any
	
	// create a hash, we'll use keys on this one
//	hashtab_t *h1 = pxHtNewHashTable ("intkey", 40, 0, 25, 50); // not as good!!
	hashtab_t *h1 = pxHtNewHashTable ("intkey", 40, 0, 25, 49);
	srand (1);
	for (int i = 0; i < NUMINTKEYS_H1; i++) {
		int n = rand();
		
		if (iHtIAddVal(h1, n, (void *)(long)n) != 1) {
			printf ("Didn't add %d\n", n);
		}
	}
	vHtPrintStats(h1);
	// Now check if the values were right
	srand (1); // restart the sequence
	errors = 0;
	for (int i = 0; i < NUMINTKEYS_H1; i++) {
		int n = rand();
		hashent_t *h = pxHtIFindEntry(h1, n);
		
		if (!h) {
			printf ("Key didn't match, key %d\n", n);
			errors++;
			continue;
		}
		if ((unsigned)h->ulValue != n) {
			printf ("Value didn't match expected value %d\n", n);
			errors++;
		}
		somevalue = n;	// we'll use this later as an exmple of presence
	}
	printresult(errors, "Filling then checking contents of new table");
	
	int total = 0;
	htIterator_t it;
	vHtInitIterator (&it, h1);
	errors = 0;
	for (hashent_t *w; (w = pxHtIteratorNext(&it));) {
		total++;
		if (total > h1->ulCurEntries) {
			printf ("visiting too many entries (looping too far)\n");
			errors++;
			break;
		}
		if (w->ulKey != w->ulValue) {
			printf ("Mismatch item %d key %d value %d\n", total, w->ulKey, w->ulValue);
			errors++;
		}
	}
	if (total != h1->ulCurEntries)
		errors++;
	printresult(errors, "Testing iterator by visiting all entries in table");

	total = 0;
	errors = 0;
	htFOREACH(h1it,w2,h1) {
		total++;
		if (total > h1->ulCurEntries) {
			printf ("visiting too many entries, aborting\n");
			errors++;
			break;
		}
		if (w2->ulKey != w2->ulValue) {
			printf ("Mismatch item %d key %d value %d\n", total, w2->ulKey, w2->ulValue);
			errors++;
		}
	}
	if (total != h1->ulCurEntries)
		errors++;
	printresult(errors, "Retesting iterator using #define macro");
	

	printresult(iHtIAddVal(h1, somevalue, NULL) != 0, "Attempted AddVal of already-present value");
	
	printresult(iHtISetVal(h1, somevalue, NULL) != 1, "Attempted SetVal of already-present value");
	
	somevalue = rand();  // never before used value
	printresult(iHtISetVal(h1, somevalue, NULL) != 1, "Attempted SetVal of non-present value");
	
	// now delete the first handful and see if they're gone
//	printf ("Hashtable has %d entries, deleting 10 of them\n", h1->curEntries);
	srand(1);
	int entries = h1->ulCurEntries;
	for (int i = 0; i < 10; i++) {
		iHtIDelete(h1, rand());
	}
	printresult(entries-10 != h1->ulCurEntries, "Deletion from table");
	entries = 0;
	srand(1);
	for (int i = 0; i < NUMINTKEYS_H1; i++) {
		entries += iHtIAddVal(h1, rand(), NULL);
	}
	printresult(entries != 10, "Checking to insure delections happened");

// -----------------------------------------------------------------------
	printf ("\nString Key Hash Tests\n");
// -----------------------------------------------------------------------

	char *samples[] = {
		"now", "is", "the", "time", "for", "all", "good", "men", "to", "come", "to", "the", "aid", "of", "their", "countrymen",
		"nowx", "isx", "thex", "timex", "forx", "allx", "goodx", "menx", "tox", "comex", "tox", "thex", "aidx", "ofx", "theirx", "countrymenx",
		"nowy", "isy", "they", "timey", "fory", "ally", "goody", "meny", "toy", "comey", "toy", "they", "aidy", "ofy", "theiry", "countrymeny"
	};

	int count, inserts = 0;
	hashtab_t *h2 = pxHtNewHashTable ("stringkey", 40, 0, 25, 47);

	count = (sizeof(samples)/sizeof(samples[0]));
	printf ("Sample count %d, adding them to the hash table\n", count);
	for (int i = 0; i < count; i++) {
		inserts += iHtSAddVal(h2, samples[i], samples[i]);
	}
	printf ("Of %d samples, %d of them were unique\n", count, inserts);
	vHtPrintStats(h2);
}

