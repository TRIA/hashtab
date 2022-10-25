//
//  main.c
//  hash
//
//  Created by Steve Bunch on 10/14/22.
//

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "hashtab.h"
#include "rsrc.h"

#define NUMINTKEYS_H1  200

int fillsamples(FILE *in, unsigned int numsamp, char *samples[], size_t buflen, char *buffer);

void printresult (int errors, const char *message)
{
	printf ("Test '%s': %s\n", message, (errors ? "FAIL" : "PASS"));
}

int main(int argc, const char * argv[]) {
	int somevalue = -1; // any of the values we put into h1
	int errors;	// used inside loops to accumulate error count, if any
	
	// create a hash, we'll use keys on this one
//	hashtab_t *h1 = pxHtNewHashTable ("intkey", 40, 0, 25, 50); // not as good!!
	hashtab_t *h1 = pxHtNewHashTable ("intkey", 40, 0, 25, 49);
	srand (1);
	for (int i = 0; i < NUMINTKEYS_H1; i++) {
		int n = rand();
		
		if (htIAddVal(h1, n, (void *)n) != 1) {
			printf ("Didn't add %d\n", n);
		}
	}
	htPrintStats(h1);
	// Now check if the values were right
	srand (1); // restart the sequence
	errors = 0;
	for (int i = 0; i < NUMINTKEYS_H1; i++) {
		int n = rand();
		hashent_t *h = htIFindEntry(h1, n);
		
		if (!h) {
			printf ("Key didn't match, key %d\n", n);
			errors++;
			continue;
		}
		if ((unsigned)h->uintvalue != n) {
			printf ("Value didn't match expected value %d\n", n);
			errors++;
		}
		somevalue = n;	// we'll use this later as an exmple of presence
	}
	printresult(errors, "Filling then checking contents of new table");
	
	int total = 0;
	htIterator_t it;
	htInitIterator (&it, h1);
	errors = 0;
	for (hashent_t *w; (w = htIteratorNext(&it));) {
		total++;
		if (total > h1->curEntries) {
			printf ("visiting too many entries (looping too far)\n");
			errors++;
			break;
		}
		if (w->key != w->uintvalue) {
			printf ("Mismatch item %d key %d value %d\n", total, w->key, w->uintvalue);
			errors++;
		}
	}
	if (total != h1->curEntries)
		errors++;
	printresult(errors, "Testing iterator by visiting all entries in table");

	total = 0;
	errors = 0;
	htFOREACH(h1it,w2,h1) {
		total++;
		if (total > h1->curEntries) {
			printf ("visiting too many entries, aborting\n");
			errors++;
			break;
		}
		if (w2->key != w2->uintvalue) {
			printf ("Mismatch item %d key %d value %d\n", total, w2->key, w2->uintvalue);
			errors++;
		}
	}
	if (total != h1->curEntries)
		errors++;
	printresult(errors, "Retesting iterator using #define macro");
	

	printresult(htIAddVal(h1, somevalue, NULL) != 0, "Attempted AddVal of already-present value");
	
	printresult(htISetVal(h1, somevalue, NULL) != 1, "Attempted SetVal of already-present value");
	
	somevalue = rand();  // never before used value
	printresult(htISetVal(h1, somevalue, NULL) != 1, "Attempted SetVal of non-present value");
	
	// now delete the first handful and see if they're gone
//	printf ("Hashtable has %d entries, deleting 10 of them\n", h1->curEntries);
	srand(1);
	int entries = h1->curEntries;
	for (int i = 0; i < 10; i++) {
		htIDelete(h1, rand());
	}
	printresult(entries-10 != h1->curEntries, "Deletion from table");
	entries = 0;
	srand(1);
	for (int i = 0; i < NUMINTKEYS_H1; i++) {
		entries += htIAddVal(h1, rand(), NULL);
	}
	printresult(entries != 10, "Checking to insure delections happened");

// -----------------------------------------------------------------------
	printf ("\nString Key Hash Tests\n");
// -----------------------------------------------------------------------

#define NUMSTRINGS 1000
#define STRINGLEN 16
	char *samples[NUMSTRINGS];
	char *space = malloc(NUMSTRINGS*(STRINGLEN+1));
	int count, inserts = 0;
	FILE *in;	// source of test data
	hashtab_t *h2 = pxHtNewHashTable ("stringkey", 40, 0, 25, 47);

	if (argc > 1) {
		in = fopen (argv[1], "r");
		if (!in) {
			printf ("Unable to open string token file '%s'\n", argv[1]);
			return (1);
		}
	} else {
		in = stdin;
	}
	count = fillsamples(in, NUMSTRINGS, samples, STRINGLEN, space);
	printf ("Sample count %d, adding them to the hash table\n", count);
	for (int i = 0; i < count; i++) {
		inserts += htSAddVal(h2, samples[i], samples[i]);
	}
	printf ("Of %d samples, %d of them were unique\n", count, inserts);
	htPrintStats(h2);
	return 0;
}

int fillsamples(FILE *input, unsigned int numsamp, char *samples[], size_t buflen, char *buffer)
{
	int len;
	
	for (int sn = 0; sn < numsamp; sn++) {
		char c;
		
		samples[sn] = buffer;
		*buffer = '\0'; // in case we hit EOF before finding any chars

		// skip delimiters
		while ((c = fgetc(input)) != EOF) {
			if (!(ispunct(c) || isblank(c) || c == '\n')) {
				break;
			}
		}
		// fill in one sample from input
		for (len = 0; len < STRINGLEN && c != EOF; len++) {
			if (ispunct(c) || isblank(c) || c == '\n') {
				break;
			}
			*buffer++ = c;
			c = fgetc(input);
		}
		if (len == 0)
			return (sn);	// hit EOF without finding anything?
		*buffer++ = '\0';

//		printf ("'%s' ", samples[sn]);
	}
	return (numsamp);
}
