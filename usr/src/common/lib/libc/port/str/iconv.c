/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/iconv.c	1.2"

#ident	"@(#)iconv:process.c	1.2.8.2"
#ident  "$Header: process.c 1.3 91/07/02 $"

/*
 * iconv() - code set conversion on the given characters.
 *
 */

#ifdef __STDC__
	#pragma weak iconv = _iconv
#endif

#include "synonyms.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#ifdef DEBUG
#include <pfmt.h>
#endif
#include <iconv.h>
#include "_iconv.h"

#define NODATA -1

/*
 * Increment a pointer to a buffer, wrapping around at the end of the
 * buffer.
 */

#define INC(X)	(X = (X+1) % KBDBUFSIZE )

#ifdef _LOCKING_SHIFTS
/*
 * Byte sequence that resets the shift state of iconv.
 */

#define RESET "not-implemented"

/*
 * Length of reset sequence 
 */

#define RESETLEN (strlen(RESET))

#endif /* _LOCKING_SHIFTS */

static int getbyte();
static void putbyte();
static void putstring();
static unsigned char rewindnodes();
static void clear();
static int process();
static int diff();


size_t
iconv(iconv_t cd, char **inbuf, size_t *inbytesleft, 
	char **outbuf, size_t *outbytesleft)
{
	struct iconv_ds *id;
	struct kbd_tab *t;

	if (cd == NULL) {
		errno = EBADF;
		return (size_t) -1;
	}

	if (cd->handle != NULL) {
		return (*(cd->conv))(cd, (const char **) inbuf, inbytesleft,
					 outbuf, outbytesleft);
	}

	id = (struct iconv_ds *) cd;
	t = id->table;

	if (inbuf == NULL || *inbuf == NULL) {
#ifdef _LOCKING_SHIFTS
		/* 
		 * set initial shift state 
		 */

		id->shift=0;

		/*
		 * put sequence needed to set initial shift state in outbuf.
		 */
		 if (inbuf != NULL && *inbuf != NULL && *outbytesleft >= 0) {
			if (*outbytesleft < RESETLEN) {
				errno=E2BIG;
				return (size_t) -1;
			}
			*outbytesleft = RESETLEN;
			strncpy(*outbuf,RESET,RESETLEN);
		 }
#else /* _LOCKING_SHIFTS */
		if (inbuf != NULL && *inbuf != NULL)
			*outbytesleft=0;
#endif /* _LOCKING_SHIFTS */
		return (size_t) 0;
	}
	id->inbuf = *inbuf;
	id->outbuf = *outbuf;

	id->insize = *inbytesleft;
	id->outsize = *outbytesleft;
	id->readptr = id->writeptr = 0;
	id->indone = 0;
	id->numconv = 0;
	id->error = 0;

	/*
	 * Clear the buffers.
	 */
	memset(id->levels, 0, 10 * sizeof(struct level));

	process(id, t, 0);
	if (id->error)
		errno=id->error;

	*inbytesleft = id->insize - id->indone;
	*inbuf = (*inbuf + id->indone);
	*outbytesleft = id->outsize - id->writeptr;
	*outbuf = (*outbuf + id->writeptr);
	return id->error ? (size_t) -1 : id->numconv;
}

/*
 * Process is a
 * recursive routine which will
 * decend the tree and pass on
 * to the next level in a composite
 * table any finished characters
 * If no data available it will 
 * return to previous level
 * will attempt to generate more data.
 */

static int
process (id, t , level)
struct iconv_ds *id;
struct kbd_tab *t;
int           level;
{
int j;
unsigned char c;
int ch;
struct cornode *n;
struct cornode *node_tab;
unsigned char buf[KBDBUFSIZE];
int inmbchar=0;		/* Are we processing a mulit-byte character?*/


    if (!t) {
	/*
	 * No table - output everthing in the buffer.
	 */

	for(j=0;(ch=getbyte(id,level)) != NODATA; j++)
		buf[j] = (unsigned char) (ch & 0x0FF);
	buf[j]='\0';

	if (id->writeptr + j <= id->outsize) {
	    strcpy(id->outbuf + id->writeptr,(char *) buf);
	    id->writeptr += j;
	    clear(id, level);
	} else {
		id->error=E2BIG;
		return -1;
	}

    } else
    if (t->t_flag == KBD_COT) {
	/*
	 * Composite table - not a real table, just pass on to the next
	 * table.
	 */
	while ((ch=getbyte(id,level)) != NODATA) {

	    c = (unsigned char) (ch & 0x0FF);
	    putbyte(id, c,level+1);
	    if (process(id, t->t_next,level+1) == -1)
		return -1;
	    clear(id, level);

	}

    } else
    if (t->t_flag & KBD_ONE) {
	/*
	 * A one to one mapping table has been defined, use that first.
	 */
	while ((ch=getbyte(id,level)) != NODATA) {

	    id->numconv++;
	    c = t->t_oneone[ch];
	    putbyte(id, c,level+1);
	    if (process(id, t->t_next,level+1) == -1)
		return -1;
	    clear(id, level);
	
	}

    } else
    if (!t->t_nodes) {
	/*
	 * table has no nodes - pass through to next table.
	 */
	while ((ch=getbyte(id,level)) != NODATA) {

	    c = (unsigned char) (ch & 0x0FF);
	    putbyte(id, c,level+1);
	    if (process(id, t->t_next,level+1) == -1)
		return -1;
	    clear(id, level);
	
	}
    
    } else {
	/*
	 * normal table.
	 */
	node_tab = t->t_nodep;
	while ((ch=getbyte(id,level)) != NODATA) {

	    c = (unsigned char) (ch & 0x0FF);
	    if (!id->levels[level].c_node) {

		if (t->t_flag & KBD_FULL) {
		    /* 
		     * This is a full map, see if the byte is with the
		     * maximum and minimum range of the map.
		     */
		    if (c > t->t_max || c < t->t_min) {
			/*
			 * If not, we're not interested, so send it to the
			 * next table.
			 */
			putbyte(id, c,level+1);
			if (process(id, t->t_next,level+1) == -1)
				return -1;
			clear(id, level);
			continue;

		    } else
			/*
			 * Byte is in map, get corresponding node number 
			 */
			j = (int) (c - t->t_min);

		} else
		    /*
		     * Sparse map, start at node zero.
		     */
		    j = 0;

		id->levels[level].c_node = &node_tab[j];

	    }

	    for(;;) {
		n = id->levels[level].c_node;
		if (n->c_val == c) {

		    /*
		     * Byte matches this node.
		     */
		    if (n->c_flag & ND_RESULT) {
			/*
			 * Last byte in many-to-[one|many] string.
			 */

			id->numconv++;
			inmbchar=0;	/* no longer processing a mbc*/
			if (n->c_flag & ND_INLINE) {
			    /*
			     * Many-to-one string.
			     */
			    putbyte(id, n->c_child,level+1);
			    if (process(id, t->t_next,level+1) == -1)
				return -1;
			    clear(id, level);
			    break;
			
			} else {
			    /*
			     * Many-to-many string.
			     */
			    putstring(id, t->t_textp,n->c_child,level+1);
			    if (process(id, t->t_next,level+1) == -1)
				return -1;
			    clear(id, level);
			    break;
			    
			}


		    }
		    /*
		     * Not the last node in a many-to-[many|one] string
		     * check next node.
		     */
		    id->levels[level].c_node = &node_tab[n->c_child];

		    inmbchar=1;		/* we're processing a multi-byte char */
		    break;

		} else {

		    /*
		     * not matching
		     */
		    if (n->c_flag & ND_LAST) {
			/*
			 * No other strings can match this prefix
			 *
			 * get the first
			 * byte that caused
			 * this tree traversal
			 */
			c = rewindnodes(id, level);
			inmbchar=0;	/* no longer processing a mbc */

			/*
			 * give it to the next level
			 * then continue from the second byte
			 * that got us here
			 *
			 * If there is an error statement in the map,
			 * replace this byte with the error string.
			 */
			if (t->t_flag & KBD_ERR) 
			    putstring(id, t->t_textp,t->t_error,level+1);
			else
			    putbyte(id, c,level+1);
			if (process(id, t->t_next,level+1) == -1)
				return -1;
			break;

		    } else {

			/*
			 * There is another string which could match the
			 * characters we have seen so far - eg with a tree
			 * such as:
			 *	      a
			 *          /   \
			 *         b     d
			 *
			 * after finding an 'a', we'd try to match the 'b'.
			 * if that didn't work, incrementing the node will 
			 * point us at the 'd'.
			 */
			id->levels[level].c_node = ++n;
			continue;

		    }
		}
	    }
	}
#ifdef _LOCKING_SHIFTS
	/*
	 * Did we run out of data while in shift mode?
	 */
	if (id->shift) {
	}
#endif /* _LOCKING_SHIFTS */
	if (inmbchar) {	/* Did we run out of data while processing a mbc? */
		id->error=EINVAL;
		return -1;
	}
    }
    return 0;
}


/*
 * Get a byte from the buffer for the current level.  returns NODATA
 * if the buffer is empty.
 * At level 0, if the buffer is empty it attempts to get more bytes from
 * the user-supplied buffer.
 */
static int
getbyte ( id , level )
struct iconv_ds *id;
int       level;
{
int n;
int j;
char buf[KBDREADBUF];
int end;
int begin;
int read_point;
unsigned char *p;

    end = id->levels[level].end;
    begin = id->levels[level].begin;
    read_point = id->levels[level].read_point;
    p = id->levels[level].input_chars;

    if (id->levels[level].must_clear) {
#ifdef DEBUG
	pfmt(stderr, MM_ERROR, ":89:Buffer Overrun. (1)\n");
#endif
	return NODATA;
    }
    if (read_point == end) {

	/*
	 * NODATA. If level 0
	 * read a byte or 2.
	 */
	if (level)
	   return NODATA;

	n=diff(begin,end);
	if (n > id->insize - id->readptr)
		n = id->insize - id->readptr;
	if (n <= 0)
	    return NODATA;
	memcpy(buf,&(id->inbuf[id->readptr]),n);
	id->readptr += n;

	/*
	 * FILL BUFFER from end
	 */
	for(j=0;j<n;j++){

	    p[end] = buf[j];
	    INC(end);

	}
	id->levels[level].end = end;

    }

    j = (int)(p[read_point] & 0x0FF);

    /*
     * increment read point
     * if i hit begin then must
     * do a clear opperation
     * before next read or
     * will get buffer overran
     */
    INC(read_point);
    id->levels[level].read_point = read_point;
    if (read_point == begin) 
	id->levels[level].must_clear = 1;

    return j;

}

/*
 * putstring uses
 * putbyte
 */

static void
putstring ( id, s , index , level )
struct iconv_ds *id;
unsigned char   *s;
unsigned int        index;
int 			    level;
{

	s += index;

	while (*s) 
		putbyte(id, *s++,level);

}

/*
 * Give the byte to the
 * next level
 */

static void
putbyte (id,  c , level )
struct iconv_ds *id;
unsigned char c;
int           level;
{
int end = id->levels[level].end;
unsigned char * p = id->levels[level].input_chars;

    p[end] = c;
    INC(id->levels[level].end);
    
}

/*
 * Clear the held characters
 * up to the read point.
 * they've been processed
 *
 * If we're at level 0, increment the count of bytes in the input buffer
 * which have been processed.
 */
static void
clear ( id, level )
struct iconv_ds *id;
int level;
{
    /* number of characters to clear */
    if (!level) {
	int n;
    	n=diff(id->levels[level].read_point, id->levels[level].begin);
	id->indone += n;
    }

    id->levels[level].must_clear = 0;
    id->levels[level].begin  =  id->levels[level].read_point;
    id->levels[level].c_node  =  NULL;


}

/*
 * We've been following a tree of bytes and have not matched one of the
 * bytes.  If there is no other string which can be matched with the
 * bytes we have matched so far, we must go back to the second byte in
 * the tree, and start trying to match that.
 */
static unsigned char
rewindnodes ( id, level )
struct iconv_ds *id;
int	      level;
{
int begin = id->levels[level].begin;
unsigned char c;

    c = id->levels[level].input_chars[begin];
    INC(begin);
    id->levels[level].read_point = begin;
    id->levels[level].must_clear = 0;
    id->levels[level].begin  =  begin;
    id->levels[level].c_node  =  NULL;
    /*
     * Inc number of bytes we have processed.
     */
    id->indone++;
    return c;

}

/*
 * Get the distance between two pointers to the buffers.
 */

static int
diff(x,y)
int x,y;
{
int n;
  n = x - y;
  if (n <= 0)
      n = KBDBUFSIZE + n;
  if (n > KBDREADBUF)
      n = KBDREADBUF;
  return n;
}

