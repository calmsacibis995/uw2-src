/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/ssm/ssm_cmblck.c	1.4"

/*
 * ssm_cmblck.c
 *	Device driver command block interface
 */

/*
 * The  command block interface is
 * designed  to  be  a generic routine interface that
 * handles all of the issues in  allocating,  reserv-
 * ing,  and  freeing command blocks.  There are four
 * routines that are associated with  the  interface:
 * alloc_cb_desc, alloc_cb_desc_cbs, get_cb, free_cb.
 * There is a struct,  cb_desc,  that  describes  the
 * nature  and  location  of  command blocks. Command
 * blocks for each device must be physically contigu-
 * ous.
 *
 * The driver must have a (int) state  field  in  its
 * command  block.  The driver must not use bit 31 of
 * the state field. The driver must know, relative to
 * the  beginning  of the cb, the offset of the state
 * field in  the  command  block.   The  driver  must
 * assure  exclusive  access to command blocks before
 * entering any of these routines.
 */

#include <util/types.h>
#include <util/debug.h>
#include <util/param.h>
#include <io/ssm/ssm_cmblck.h>
#include <mem/kmem.h>

/*
 * struct cb_desc * alloc_cb_desc(caddr_t, short, short, short)
 *
 * Calling/Exit State:
 *	caddr_t cbs	-	command blocks
 *	short count	-	number of command blocks
 *	short size	-	size of one command block
 *	short fld_offset -	relative offset of state field
 *
 *	The routine alloc_cb_desc allocates and fills  out
 *	a command block descriptor.  It needs as arguments
 *	a pointer to a block of contiguous command blocks,
 *	the  number  of command blocks, the size of a com-
 *	mand block and the offset of the state field.   It
 *	returns  a  pointer  to an allocated command block
 *	descriptor.
 */
struct cb_desc *
alloc_cb_desc(caddr_t cbs, short count, short size, short fld_offset)
{
	struct cb_desc *cbdp;
	
	ASSERT(count >= 0);
	/*
	 *+ In alloc_cb_desc, an attempt has been made to allocate a negative
	 *+ number of command descriptor blocks.  Corrective action:  fix the
	  *+ kernel routine calling this function.
	 */

	ASSERT(size >= 0);
	/*
	 *+ In alloc_cb_desc, an attempt has been made to allocate command
	 *+ descriptor blocks having a negative size.  Corrective action:
	 *+ fix the kernel routine calling this function.
	 */

	cbdp = (struct cb_desc *) kmem_zalloc(sizeof(struct cb_desc), KM_SLEEP);

	cbdp->cb_cbs = cbs;
	cbdp->cb_count = count;
	cbdp->cb_size = size;
	cbdp->cb_state_offset = fld_offset;

	return (cbdp);
}

/*
 * caddr_t get_cb(struct cb_desc *)
 *
 * Calling/Exit State:
 *	The get_cb routine is responsible for  looking  at
 *	bit  31  of  the state field of each cb associated
 *	with cbdp until a free cb is  found  (bit  31  not
 *	set).  If a free cb is found get_cb sets bit 31 of
 *	the state field and returns the address of the cb.
 *	If there were no cb's available the driver returns
 *	NULL. If there was an error then get_cb panics.
 */
caddr_t 
get_cb(struct cb_desc *cbdp)
{
	int x;
	caddr_t	cb_point;

	ASSERT(cbdp->cb_count >= 0);
	/*
	 *+ In the command block passed to routine get_cb, the descriptor's
	 *+ count of command blocks is negative.  
	 */

	ASSERT(cbdp->cb_state_offset >= 0);
	/*
	 *+ In the command block passed to routine get_cb, the descriptor's
	 *+ offset to the state field is negative.  
	 */

	ASSERT(cbdp->cb_cbs != NULL);
	/*
	 *+ In the command block passed to routine get_cb, the descriptor's
	 *+ pointer to command blocks is NULL.  
	 */

	cb_point = cbdp->cb_cbs;
	for(x=0; x<cbdp->cb_count; x++, cb_point += cbdp->cb_size) {
		if (*(int *) ((int) cb_point + cbdp->cb_state_offset) & CB_BUSY) {
			continue;
		}
		*(int *) ((int) cb_point + cbdp->cb_state_offset) |= CB_BUSY;
			return (cb_point);
	}
	return (NULL);
}

/*
 * void free_cb(caddr_t, struct cb_desc *)
 *
 * Calling/Exit State:
 *	The free_cb routine clears bit 31 of the cb it  is
 *	passed. The routine always returns 0. If there was
 *	an error free_cb panics.
 */
void
free_cb(caddr_t cb, struct cb_desc *cbdp)
{
	ASSERT(cbdp->cb_state_offset >= 0);
	/*
	 *+ In the command block passed to routine free_cb, the descriptor's
	 *+ offset to the state field is negative. 
	 */

	ASSERT(cb != NULL);
	/*
	 *+ In the command block passed to routine free_cb, the descriptor's
	 *+ pointer to command blocks is NULL. 
	 */

	*(int *) ((int) cb + cbdp->cb_state_offset) &= ~CB_BUSY;
}
