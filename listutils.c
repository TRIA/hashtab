/*
 *  listutils.c
 *  PNA-servers
 *
 *  Created by Steve Bunch on 6/22/10.
 *  Copyright 2010 TRIA Network Systems. All rights reserved.
 *
 */

#include <stdlib.h>				// just for NULL, believe it or not...
#include "listutils.h"

// add a list element at the end (tail) of the list
void lAppend (dlList_t *head, dlList_t *entry)
{
	dlList_t *oldtail = head->left;
	
	entry->right = head;		// prepare new entry to be new tail
	entry->left = oldtail;
	oldtail->right = entry;		// point previous tail's right to entry
	head->left = entry;			// point list head left at new tail
	dlListCheck(head);
}

// add a list element to the front of the list
void lInsert (dlList_t *head, dlList_t *entry)
{
	dlList_t *oldfront = head->right;
	
	entry->right = oldfront;	// prepare the entry to be new front
	entry->left = head;
	oldfront->left = entry;		// point previous front back at entry
	head->right = entry;		// point head right at new front
	dlListCheck(head);
}

// remove front element from list and return it
dlList_t *lGetFirst (dlList_t *head)
{
	dlList_t *front;
	
	if (head == 0)		// No list
		return (NULL);
	front = head->right;
	if (head == front)	// Nothing in the list
		return (NULL);
	head->right = front->right;	// snip out of right link path
	front->right->left = head;	// snip out of left link path
	front->right = front->left = NULL;
	dlListCheck(head);
	return(front);
}

// remove back element from list and return it
dlList_t *lGetLast (dlList_t *head)
{
	dlList_t *back = head->left;
	
	if (head == back)			// Nothing in the list
		return (NULL);
	head->left = back->left;	// snip out of left link path
	back->left->right = head;	// snip out of right link path
	back->right = back->left = NULL;
	dlListCheck(head);
	return(back);
}

// Delete an element from the list.  Can be anywhere, but if it's
// the head of an empty list, return NULL (common error, let's catch it here)
dlList_t *lDelete (dlList_t *entry)
{
	dlList_t *left = entry->left, *right = entry->right;
	
	if (left == NULL || entry == right)	// already removed, or empty list?
		return (NULL);
	left->right = right;
	right->left = left;
	// to guard against misuse
	entry->left = entry->right = NULL;
	return (entry);
}

#ifndef dlListCheck				// if it's a macro, we don't need this
// sanity-check list links on a list -- may crash, but better here than randomly
int dlListCheck(dlList_t *head)
{
	char *killme = NULL;
	dlList_t *right, *prev;
	int l;						// will count length of list
	
	// check for NULLs
	if (head == NULL || head->right == NULL || head->left == NULL)
		*killme = 1;
	// check forward links and count how far til we get back to head
	prev = head;
	right = head->right;
	for (l = 0; right != head; l++) {
		// for each entry, check to make sure it points back to where we came from
		if (right->left != prev)
			*killme = 2;
		prev = right;
		right = prev->right;
	}
	// right is now matching head.  Make sure head points left to prev.
	if (head->left != prev)
		*killme = 3;
	return (l);
}
#endif //dlListCheck
