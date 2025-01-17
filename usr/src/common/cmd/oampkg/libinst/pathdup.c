/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)oampkg:common/cmd/oampkg/libinst/pathdup.c	1.1.7.4"
#ident  "$Header: pathdup.c 1.2 91/06/27 $"

#include <limits.h>
#include <string.h>

#define ERR_MEMORY	"uxpkgtools:6:memory allocation failure, errno=%d"

extern int	errno;
extern void	*calloc(), 
		progerr(),
		quit(),
		free();

/* using factor of eight limits maximum 
 * memory fragmentation to 12.5%
 */
#define MEMSIZ	PATH_MAX*8
#define NULL	0

struct dup {
	char	mem[MEMSIZ];
	struct dup *next;
};

static struct dup 
		*head, *tail;
static int	size;

char *
pathdup(s)
char	*s;
{
	struct dup	*new;
	char	*pt;
	int	n;

	if(s == NULL) {
		if(head == NULL) {
			n = 0;
			size = (-1);
		} else {
			/* free all memory used except initial structure */
			tail = head->next;
			while(tail) {
				new = tail->next;
				free(tail);
				tail = new;
			}
			tail = head;
			size = MEMSIZ;
			return(NULL);
		}
	} else
		n = strlen(s) + 1;

	if(size < n) {
		/* need more memory */
		new = (struct dup *) calloc(1, sizeof(struct dup));
		if(new == NULL) {
			progerr(ERR_MEMORY, errno);
			quit(99);
		}
		if(head == NULL)
			head = new;
		else
			tail->next = new;
		tail = new;
		size = MEMSIZ;
	}

	pt = &tail->mem[MEMSIZ-size];
	size -= n;

	if(s != NULL)
		(void) strcpy(pt, s);
	return(pt);
}
