/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)sh:common/cmd/sh/hash.c	1.3.7.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/sh/hash.c,v 1.1 91/02/28 20:08:30 ccs Exp $"

#include	"hash.h"
#include	"defs.h"

#define STRCMP(A, B)	(cf(A, B) != 0)
#define FACTOR 	 	035761254233	/* Magic multiplication factor */
#define TABLENGTH	64		/* must be multiple of 2 */
#define LOG2LEN		6		/* log2 of TABLENGTH */


/*
    NOTE: The following algorithm only works on machines where
    the results of multiplying two integers is the least
    significant part of the double word integer required to hold
    the result.  It is adapted from Knuth, Volume 3, section 6.4.
*/

#define hash(str)		(int)(((unsigned)(crunch(str) * FACTOR)) >> shift)
struct node
{
	ENTRY item;
	struct node *next;
};

static struct node	**last;
static struct node	*next;
static struct node 	**table;

static unsigned int 	bitsper;		/* Bits per byte */
static unsigned int	shift;

static unsigned int crunch();

void
hcreat()
{
	unsigned char c = (unsigned char)~((char)0);/* A byte full of 1's */
	int j;

	table = (struct node **)alloc(TABLENGTH * sizeof(struct node *));

	for (j=0; j < TABLENGTH; ++j)  
	{
		table[j] = 0;
	}

	bitsper = 0;

	while (c)		
	{
		c = (unsigned int)c >> 1;
		bitsper++;
	}

	shift = (bitsper * sizeof(int)) - LOG2LEN;
}


void
hscan(uscan)	
	void	(*uscan)();
{
	struct node		*p, *nxt;
	int				j;

	for (j=0; j < TABLENGTH; ++j)
	{
		p = table[j];
		while (p)
		{
			nxt = p->next;
			(*uscan)(&p->item);
			p = nxt;
		}
	}
}



ENTRY *
hfind(str)
	unsigned char	*str;
{
	struct node 	*p;
	struct node 	**q;
	unsigned int 	i;
	int 			res;		

	i = hash(str);

	if(table[i] == 0)
	{			
		last = &table[i];
		next = 0;
		return(0);
	}
	else 
	{
		q = &table[i];
		p = table[i];
		while (p != 0 && (res = STRCMP(str, p->item.key))) 
		{
			q = &(p->next);
			p = p->next;
		}

		if (p != 0 && res == 0)	
			return(&(p->item));
		else
		{
			last = q;
			next = p;
			return(0);
		}
	}
}

ENTRY *
henter(item)
	ENTRY item;
{
	struct node	*p = (struct node *)alloc(sizeof(struct node));

	p->item = item;
	*last = p;
	p->next = next;
	return(&(p->item));
}


static unsigned int 
crunch(key)	
	unsigned char	*key;
{
	unsigned int 	sum = 0;	
	int s;

	for (s = 0; *key; s++)				/* Simply add up the bytes */
		sum += *key++;

	return(sum + s);
}

