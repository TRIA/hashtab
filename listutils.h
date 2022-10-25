/*
 *  listutils.h
 *  PNA-servers
 *
 *  Created by Steve Bunch on 6/22/10.
 *  Copyright 2010 TRIA Network Systems. All rights reserved.
 *
 */

#ifndef _LISTUTILS_H_
#define _LISTUTILS_H_

typedef struct llink {
	struct llink *left;
	struct llink *right;
} dlList_t;


/* List utilities, support doubly-linked circular lists.
 * Circular lists include the head in the list.  Circular
 * list heads will point at themselves when there are no
 * entries.
 * Last element in a circular list right-points at the head.  
 * Head of a circular list left-points at the last item.
 * You can add entries to either end.
 * You can delete from the middle; it's a bad idea to delete
 * the list head.  (If you try to delete an item that points
 * to itself, we notice and return NULL.)
 *
 * Note that although it's called "head", the first argument
 * to lAppend or lInsert does not actually have to be the list
 * head, it can be an element. It's the element that we'll put
 * the new list entry before, or after, respectively, so if 
 * it's NOT the list head, the naming is backwards -- sorry!
 */

#define LLINKSINIT(l)		((dlList_t *)(l))->left = ((dlList_t *)(l))->right = ((dlList_t *)(l))
#define LISTEMPTY(l)		(l == 0 || ((dlList_t *)(l)) == ((dlList_t *)(l))->right)
#define LISTNEXT(l)			(((dlList_t *)(l))->right)

void lAppend (dlList_t *head, dlList_t *entry);
void lInsert (dlList_t *head, dlList_t *entry);
dlList_t *lGetFirst (dlList_t *head);	// always from front
dlList_t *lGetLast (dlList_t *head);	// always from tail
dlList_t *lDelete (dlList_t *entry);	// if it's the list head, return NULL

#define dlListCheck(x)					// comment this out when debugging lists
#ifndef dlListCheck
int dlListCheck(dlList_t *head);		// sanity-check links for correctness
#endif // dlListCheck

#endif // _LISTUTILS_H_
