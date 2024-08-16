/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _FS_BUF_H	/* wrapper symbol for kernel use */
#define _FS_BUF_H	/* subject to change without notice */

#ident	"@(#)kern:fs/buf.h	1.77"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ifdef _KERNEL_HEADERS

#include <mem/kmem.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/kmem.h>		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * The buf structure, also called a buffer header, is the basic data
 * structure for block I/O transfers.
 *
 * Each block I/O or physical (direct) I/O transfer has an associated
 * buffer header structure.  This structure contains control and status
 * information for the transfer.  Buffer headers are passed to block
 * drivers' strategy(D2) routines.  They may also be allocated by the
 * driver itself using getrbuf(D3), ngeteblk(D3) or geteblk(D3).
 *
 * Associated with each buffer header is a data buffer which holds the
 * data for the I/O.  The data buffer may be in kernel data space, or
 * it may be simply a list of physical memory pages.  It may also be
 * a portion of user data space locked into memory, in the physical I/O
 * case [see physiock(D3)].
 *
 * Do not depend on the size of the buf structure when writing a driver
 * (or any other module which needs binary compatibility).  In particular,
 * this means you must only allocate buf structures via the interfaces in
 * buf.h (for example, getrbuf).  Static allocations are not allowed.
 *
 * It is important to note that a buffer header may be linked onto
 * multiple lists simultaneously, and is also passed back to the system
 * when the driver is done with it.  Because of this, most of the members
 * in the buffer header cannot be changed by the driver, even when the
 * buffer header is in one of the driver's work lists.
 *
 * To understand the rules for using various members of the buf structure,
 * it is necessary to first understand the various agents which handle
 * a buffer, and how control of the buffer is passed amongst these agents.
 *
 * The first agent is the "creator".  The creator agent acquires the
 * buffer (through interfaces such as getblk, getrbuf, ngeteblk, and
 * pageio_setup) and initializes it.  Some of the initialization is done
 * inside the interface; some is done by the caller.  The creator is
 * often part of a filesystem module or a kernel filter routine, but it
 * may also be a driver routine which calls getrbuf, geteblk, or ngeteblk.
 *
 * Another agent is the "I/O handler".  The I/O handler is the part of a
 * device driver, starting from its strategy entry point, which receives a
 * buffer as an I/O request, handles the transfer of data and/or errors, and
 * indicates I/O completion by calling biodone(D3).
 *
 * An additional type of agent which sometimes gets involved in the
 * process, often transparently, is a kernel filter routine.  A filter
 * routine receives a buffer which either has already been passed to an
 * I/O handler or is about to be, performs some transformations on either
 * the buffer itself or newly created buffer(s) based on the original
 * buffer, then passes the transformed buffer(s) on to (or back to) the
 * I/O handler or the next filter.
 *
 * Control of a buffer is handed off from one agent to another over the
 * life of the buffer.  At any given time, exactly one agent has control
 * of the buffer.  Only the controlling agent may do anything with the
 * buffer besides waiting for I/O completion with biowait(D3) or
 * biowait_sig(D3).
 *
 * The creator controls the buffer initially.  When the driver strategy
 * routine is called, control transfers to the I/O handler.  Control
 * remains with the I/O handler until it calls biodone.  For synchronous
 * I/O, the creator calls biowait; when biowait returns, control returns
 * to the creator.  If a filter routine is called, control transfers to
 * the filter routine until it calls the next-level strategy routine.
 * During I/O completion processing, if an iodone routine is called
 * (b_iodone was non-NULL), control transfers to that routine, which is
 * considered to be part of the agent that set b_iodone in the first
 * place.
 *
 * Many buffer fields are only allowed to be modified by certain agents.
 * These and other restrictions are listed below, in the descriptions of
 * the individual fields.  References to "the driver" refer to the whole
 * driver, no matter which agent.  Where a field is described as being
 * preserved by an agent, this means that either the agent does not
 * change the field, or that before giving up control of the buffer, the
 * agent restores the field's value to the value it had when the agent
 * first acquired control.
 *
 * Drivers are only allowed to access certain buffer fields.  Accesses
 * by a driver to any other field are illegal and may not continue to
 * work in a subsequent release.  The following fields may be accessed
 * by the driver:
 *
 * b_flags
 *	This is a bitmask of flag bits which reflect buffer status and
 *	control flags.  This field may not be directly assigned by any
 *	agent; it is only legal to set or clear specific bits.
 *
 *	The driver may only access some of these flag bits.  The following
 *	flags may be accessed by the driver:
 *
 *	B_PAGEIO	If set, the data buffer is represented as a page
 *			list.  This means that b_un.b_addr is not a virtual
 *			address but is instead the offset into the first
 *			page of a list of one or more physical pages.
 *			This list of pages is accessible via the getnextpg
 *			function.  The pages will be in contiguous device
 *			block order, starting from the first block, given by
 *			b_blkno/b_blkoff.  If B_PAGEIO is not set, b_un.b_addr
 *			is a virtual address; it is a global kernel virtual
 *			address if B_PHYS is not set, or a user virtual
 *			address if B_PHYS is set.  This flag may not be
 *			changed except by kernel utility routines which
 *			create, map, or unmap the buffer.  If a driver does
 *			not have D_NOBRKUP set in its devflag, it will never
 *			see a buffer with B_PAGEIO set.
 *
 *	B_PHYS		Indicates that the data buffer is in user virtual
 *			space.  b_un.b_addr contains the starting user
 *			virtual address of the data buffer.  The data buffer
 *			and its virtual addresses are locked in memory
 *			so that accesses are guaranteed to succeed.  The
 *			user virtual address may not be directly accessed;
 *			it must either be mapped into a kernel virtual
 *			address using bp_mapin(D3), converted to physical
 *			addresses using vtop(D3) and b_proc, or copied
 *			to/from kernel space using copyin(D3)/copyout(D3)
 *			(the copyin/copyout routines may only be used in
 *			the context of the user process which initiated
 *			the I/O).  B_PHYS and B_PAGEIO will never be set
 *			simultaneously.  This flag may not be changed except
 *			by kernel utility routines which create, map, or
 *			unmap the buffer.  If a driver does not call
 *			physiock(D3) and does not have D_NOBRKUP set in its
 *			devflag, it will never see a buffer with B_PHYS set.
 *
 *	B_READ		Indicates that data are to be transferred from
 *			the peripheral device into main memory.  This
 *			flag may only be changed by the creator agent.
 *
 *	B_WRITE		Indicates that data are to be transferred from
 *			main memory to the peripheral device.  B_WRITE
 *			is a pseudo-flag that occupies the same bit
 *			location as B_READ.  B_WRITE cannot be directly
 *			tested; it is only detected as the absence of
 *			B_READ (!(bp->b_flags&B_READ)); it can only be
 *			"set" by clearing B_READ.
 *
 * b_forw/b_back
 *	These fields can be used to link the buffer onto a doubly-linked
 *	list.  They may only be used by the creator agent (or, if the
 *	creator is in a driver, by the whole driver), and only if the
 *	buffer was created by getrbuf.
 *
 * av_forw/av_back
 *	These fields can be used to link the buffer onto a doubly-linked
 *	list.  The driver may change these anytime it controls the buffer.
 *	These fields must be preserved by any filters.
 *
 * b_bufsize
 *	This field contains the size in bytes of the allocated buffer.
 *	This field may not be changed except by kernel utility routines
 *	which create buffers, or by the creator agent if the buffer was
 *	created by getrbuf.
 *
 * b_bcount
 *	This field specifies the number of bytes to be transferred.  It
 *	will be set to the total byte count (which should always be a
 *	multiple of NBPSCTR) upon initial entry to the I/O handler or a
 *	filter.  This field may be changed by the creator or the I/O
 *	handler.
 *
 * b_edev
 *	This field contains the external device number of the device.
 *	Only the creator may change this member.
 *
 * b_blkno
 *	This field specifies the first logical block on the device which
 *	is to be accessed.  One block equals NBPSCTR bytes.  The driver
 *	may have to convert this logical block number to a physical location
 *	such as a cylinder, track, and sector of a disk.  Only the creator
 *	may change this member.
 *
 * b_blkoff
 *	This field specifies the byte offset within the block given by
 *	b_blkno of the beginning of the transfer.  This will always be
 *	less than NBPSCTR.  Only the creator may change this member.
 *	Unless the driver indicates that it understands b_blkoff by
 *	setting D_BLKOFF in its devflag and/or setting BCB_BLKOFF in a
 *	bcb_t passed to buf_breakup(D3), b_blkoff will always be zero
 *	and may be ignored.
 *
 * b_un.b_addr
 *	This field indicates the start of the data buffer.  It is either
 *	a virtual address or an offset into the first page of a page list
 *	(see B_PAGEIO and B_PHYS for more details).  The creator or the
 *	I/O handler may change this member.
 *
 * b_proc
 *	When B_PHYS is set, b_proc identifies the process which contains the
 *	data buffer pointed to by the user virtual address in b_un.b_addr.
 *	When B_PHYS is not set, b_proc will be NULL.  This field can thus
 *	be used as the second argument to vtop to convert user virtual
 *	addresses from b_un.b_addr into physical addresses.  When B_PAGEIO
 *	is set, b_proc is undefined and should be ignored.  This field
 *	may not be changed except by kernel utility routines which create
 *	buffers.
 *
 * b_resid
 *	This field indicates the number of bytes not transferred, usually
 *	because of an error (a value of zero indicates a successful
 *	complete transfer).  The I/O handler must set this member before
 *	it calls biodone.
 *
 * b_start
 *	This field is used to hold the time the I/O request was started
 *	(as obtained by drv_getparm(LBOLT)).  The I/O handler may set it
 *	and use it upon I/O completion to compute response time metrics.
 *
 * b_iodone
 *	This field identifies a specific routine to be called when the
 *	I/O has completed.  If it is non-NULL, biodone will call
 *	(*b_iodone) instead of doing the normal completion processing.
 *	Any agent may change this member, but it must restore the
 *	previous value before calling biodone (and thus relinquish
 *	control of the buffer); for the creator, the previous value
 *	will always be NULL.  This protocol allows for "stacking" of
 *	iodone routines (particularly useful for filters).  Each agent
 *	saves the old value, sets b_iodone to its iodone routine, and
 *	hands the buffer off to the next filter or the I/O handler.
 *	On completion, the I/O handler calls biodone, and each iodone
 *	routine performs its final processing, restores b_iodone to the
 *	saved value, and calls biodone again, thus invoking the next
 *	iodone routine.
 *
 * b_misc
 *	This is a miscellaneous field for use by the controlling agent.
 *	One common use is in conjunction with b_iodone, to help in
 *	saving the previous b_iodone value.  Typically, the previous
 *	values of b_iodone and b_misc would be saved in a structure,
 *	b_misc set to point to this structure, and b_iodone set to point
 *	to the new iodone routine.  This field may only be used by the
 *	controlling agent.  If the controlling agent is the creator,
 *	it may modify b_misc directly; otherwise it must preserve the
 *	original value before returning the buffer to another agent.
 *
 * b_priv/b_priv2
 *	These fields are private fields for use by the driver.  No other
 *	agents interpret or change them.
 *
 * The following fields may not be used by drivers:
 *
 * b_pages
 *	When B_PAGEIO is set, or when B_REMAPPED is set and B_WASPHYS is
 *	clear, this field points to a linked list of pages which contain
 *	the data buffer.  Otherwise, it is undefined.  This field may not
 *	be changed except by kernel utility routines.
 *
 * b_numpages
 *	When B_PAGEIO is set, or when B_REMAPPED is set and B_WASPHYS is
 *	clear, this field indicates the number of pages on b_pages which
 *	are a part of the I/O.  Otherwise, it is undefined.  This field
 *	may not be changed except by kernel utility routines.
 *
 * b_writestrat
 *	This field, if non-NULL, specifies a routine to be called from
 *	bwrite(), when the system writes buffers, instead of the driver's
 *	strategy routine.  This routine, if specified, must eventually
 *	call the driver's strategy routine.  Only the creator may change
 *	this member.
 *
 * b_orig_addr
 *	This field is used by bp_mapin/bp_mapout to save the original
 *	value of b_un.b_addr when a B_PHYS buffer is remapped.  It is
 *	valid only when B_WASPHYS is set.
 *
 * b_orig_proc
 *	This field is used by bp_mapin/bp_mapout to save the original
 *	value of b_proc whan a B_PHYS buffer is remapped.  It is valid
 *	only when B_WASPHYS is set.
 *
 * b_odev
 *	This field is used to support compatibility with old device
 *	drivers.  It is only used in selected places that are needed
 *	for this purpose, and should not generally be used.
 *
 *************************************************************************
 *
 * Buffer pool implementation details:
 *
 * Each buffer in the pool is usually doubly linked into 2 lists:
 *    1) the device with which it is currently associated (always)
 *    2) on a list of blocks available for allocation for other use (usually).
 *
 * The latter list is kept in last-used order, and the two lists are
 * doubly linked to make it easy to remove a buffer from one list when
 * it was found by looking through the other.
 *
 * A buffer is on the available list, and is liable to be reassigned to
 * another disk block, if and only if it is not marked B_BUSY.  When a
 * buffer is busy, the available-list pointers can be used for other
 * purposes.  Most drivers use the forward ptr as a link in their I/O
 * active queue.  A buffer header contains all the information required
 * to perform I/O.  Most of the routines which manipulate buffers are
 * in bio.c.
 *
 * There are two synchronization objects in a buffer.  The first (b_avail)
 * is a sleep lock which single threads access to a buffer.  The sleep lock
 * is obtained implicitly when a buffer is to be returned to a user via
 * getblk() or bread().  When a buffer is released via brelse()--i.e.
 * the buffer is made available to other LWPs--the lock is released.
 * Again, releasing the lock is a side effect of the brelse().
 *
 * The other synchronization object (b_iowait) is used to both signal
 * I/O completion and to wait for I/O completion.  It's awaited in biowait()
 * after an LWP initiates an I/O request and signaled in biodone() when
 * the I/O request is complete.
 *
 * The buffer header flag B_DONE is required in addition to b_iowait,
 * in order to remember that an I/O operation for this buffer has been
 * completed in the past.  It is set on the completion of an I/O operation
 * or when a LWP releases a buffer through bdwrite().  Buffers which do
 * not have B_DONE set in the buffer header do not contain valid data.
 * LWPs must either read in data for the buffer through bread() or place
 * data into the buffer and either bwrite(), bawrite(), or bdwrite() the
 * buffer.
 *
 * Buffers which have B_DONE set in the buffer header may (depending
 * on whether B_ERROR & bp->b_flags is set) have valid data.
 * These buffers do not need to be filled from backing store to be used.
 */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Rather than define the buf struct here, we create #define's for
 * each field, and let the family-specific buf_f.h use them to construct
 * the actual structure.  This allows family-specific preservation of
 * offsets of driver-accessible fields, for binary compatibility purposes.
 *
 * The struct buf declaration in buf_f.h must include all of the fields
 * listed here, and should only include additional fields if they are
 * for binary compatibility purposes (e.g. padding fields).
 */

typedef struct buf buf_t;

#define _B_FLAGS	uint_t b_flags;		/* misc flags; see below */
#define _B_FORW		union { \
			   buf_t *_b_forw; \
			   struct proc *_b_orig_proc; \
			} _b_un1;	/* one of the following two: */
#define b_forw		_b_un1._b_forw		/* hash list forward link */
#define b_orig_proc	_b_un1._b_orig_proc	/* used by bp_mapin/bp_mapout:
						 * saved b_proc when remapped
						 * B_PHYS */
#define _B_BACK		buf_t *b_back;		/* hash list backward link */
#define _B_AVFORW	buf_t *av_forw;		/* free list link pointers, */
#define _B_AVBACK	buf_t *av_back;		/*     if not B_BUSY  */
#define _B_BUFSIZE	long b_bufsize;		/* size of allocated buffer */
#define _B_BCOUNT	uint_t b_bcount;	/* transfer count (in bytes) */
#define _B_EDEV		dev_t b_edev;		/* device major/minor number */
#define _B_BLKNO	daddr_t b_blkno;	/* block # on device */
#define _B_BLKOFF	ushort_t b_blkoff;	/* byte offset within block */
#define _B_ADDRTYPE	uchar_t b_addrtype;	/* type of address in b_addr */
#define _B_ORIG_TYPE	uchar_t b_orig_type;	/* original b_addrtype,
						 * saved by buf_breakup */
#define _B_ADDR		union { \
			   caddr_t b_addr;	/* address of buffer data */ \
			   void *b_ptr;		/* generic pointer to data */ \
			   daddr_t *b_daddr;	/* data as disk block ptrs */ \
			   paddr_t b_paddr;	/* physical address */ \
			   struct ba_scgth *b_scgth; /* scatter/gather list */ \
			} b_un;
#define _B_PROC		struct proc *b_proc;	/* process doing physical I/O */
#define _B_PAGES	struct page *b_pages;	/* page list for B_PAGEIO */
#define _B_NUMPAGES	uchar_t b_numpages;	/* # pages in b_pages for I/O */
#define _B_SCGTH_COUNT	union { \
			   ushort_t _b_scgth_count; \
			   o_dev_t _b_odev; \
			} _b_un4;	/* one of the following two: */
#define b_scgth_count	_b_un4._b_scgth_count	/* # entries in scatter/gather
						 * list when b_addrtype is
						 * BA_SCGTH */
#define b_odev		_b_un4._b_odev		/* binary compat dev # */
#define _B_ORIG_ADDR	caddr_t b_orig_addr;	/* used by bp_mapin/bp_mapout:
						 * saved b_addr when remapped
						 * B_PHYS */
#define _B_ERROR	int b_error;		/* error number (if B_ERROR) */
#define _B_RESID	uint_t b_resid;		/* # bytes not transferred */
#define _B_MISC		void *b_misc;		/* miscellaneous data */
#define _B_PRIV		union { \
			   void *un_ptr; \
			   int un_int; \
			} b_priv;		/* driver private data */
#define _B_PRIV2	union { \
			   void *un_ptr; \
			   int un_int; \
			   long un_long; \
			   daddr_t un_daddr; \
			} b_priv2;		/* driver private data */
#define _B_CHILDCNT	int b_childcnt;		/* reserved for buf_breakup */
#define _B_IODONE	void (*b_iodone)();	/* func called by biodone \
						 * if non-NULL */
#define _B_WRITESTRAT	void (*b_writestrat)();	/* function called by bwrite \
						 * if non-NULL */
#define _B_START	clock_t b_start;	/* request start time */
#define _B_RELTIME	clock_t b_reltime;	/* previous release time */
#define _B_IOWAIT	event_t	b_iowait;	/* signals completion of I/O */
#define _B_AVAIL	sleep_t	b_avail;	/* held by buffer owner */
	/*
	 * End of struct buf fields
	 */

#ifdef _KERNEL_HEADERS

#include <fs/buf_f.h>	/* PORTABILITY */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/buf_f.h>	/* PORTABILITY */

#endif /* _KERNEL_HEADERS */

#define paddr(bp)	(paddr_t)((bp)->b_un.b_addr)
#define b_addrp         b_un.b_ptr

/*
 *	These flags are kept in b_flags.
 *	B_BUSY and B_WANTED are present for compatibility reasons only.
 *	Neither flag is used by bio.c; also, these flags may not be used
 *	by new DDI/DKI compliant drivers.
 */
#define B_WRITE		0x0000	        /* non-read pseudo-flag              */
#define B_READ		0x0001	        /* read when I/O occurs              */
#define B_DONE		0x0002	        /* transfer finished		     */
#define B_ERROR		0x0004	        /* transfer aborted		     */
#define B_BUSY		0x0008	        /* not on av_forw/back free list     */
#define B_PHYS		0x0010		/* physical I/O from user pages	     */
#define B_WASPHYS	0x0020		/* buffer was B_PHYS before being
					 * remapped			     */
#define B_WANTED	0x0040	        /* issue wakeup when BUSY goes off   */
#define B_AGE		0x0080	        /* delayed write for correct aging   */
#define B_ASYNC		0x0100	        /* don't wait for I/O completion     */
#define B_DELWRI	0x0200	        /* delay write until buffer needed   */
#define B_MARKER	0x0400	        /* marker buffer to walk through the
					 * free list			     */
#define B_STALE		0x0800	        /* buffer lacks identity             */
#define	B_PAGEIO	0x10000	        /* do I/O to pages on bp->p_pages    */
#define B_DONTNEED	0x20000		/* after write, need not be cached   */
#define	B_TAPE		0x40000	        /* this is a magtape (no bdwrite)    */
#define B_DELAYDONE	0x80000		/* delay biodone() until base level  */
#define	B_REMAPPED	0x100000	/* buffer is kernel addressable      */
#define B_PRIVADDR	0x200000	/* buffer owns bp_mapin private addr */
#define	B_PAGEOUT	0x400000	/* used for reclaim accounting in 
					 * pvn_done().			     */
#define	B_CACHE		0x800000	/* did bread find us in the cache ?  */
#define	B_INVAL		0x1000000	/* invalidate pages after I/O done   */
#define	B_FORCE		0x2000000	/* semi-permanent removal from cache */
#define B_PARTIAL	0x8000000	/* partial buffer; don't do pvn_done */
#define	B_BAD		0x10000000	/* bad block revectoring in progress */
#define B_KERNBUF	0x20000000	/* buffer header was allocated by the
					 * kernel, so we know it's the latest
					 * version of the structure	     */
#define B_PRIVBLK	0x40000000	/* private buf for pageout/bmap	     */
#define B_TRYLOCK	0x80000000	/* don't block for locks in
					 * pvn_getdirty			     */

/*
 * Address types for b_addrtype.
 */
#define BA_KVIRT	(1 << 0)	/* contiguous kernel virtual --
					 * b_addr: base kernel virtual address
					 * b_proc: NULL
					 */
#define BA_UVIRT	(1 << 1)	/* contiguous user virtual --
					 * b_addr: base user virtual address
					 * b_proc: user process
					 */
#define BA_PAGELIST	(1 << 2)	/* list of physical pages --
					 * b_addr: offset into first page
					 * b_pages: linked list of pages
					 * b_numpages: number of pages for xfer
					 * [use pptophys/getnextpg to access]
					 */
#define BA_PHYS		(1 << 3)	/* contiguous physical --
					 * b_addr: base physical address
					 */
#define BA_SCGTH	(1 << 4)	/* physical scatter/gather list --
					 * b_addr: pointer to ba_scgth_t array
					 * b_scgth_count: # entries in array
					 */

/*
 * Scatter/Gather List entry.
 */
typedef struct ba_scgth {
	paddr_t		sg_base;	/* base physical address */
	size_t		sg_size;	/* size, in bytes, of this piece */
} ba_scgth_t;

/*
 * Breakup Control Block.
 *
 * Passed to buf_breakup() to indicate desired properties.
 */
typedef struct bcb {
	uchar_t		bcb_addrtypes;	/* type(s) of addresses accepted;
					 * must include at least one of
					 * BA_KVIRT, BA_PHYS */
	uchar_t		bcb_flags;	/* misc flags; see below */
	size_t		bcb_max_xfer;	/* maximum transfer length --
					 * total xfer if not BA_SCGTH,
					 * else per ba_scgth_t entry */
	size_t		bcb_granularity;/* minimum acceptable offset/size
					 * unit (sector size, for disks) */
	physreq_t	*bcb_physreqp;	/* physical alignment requirements */
} bcb_t;

/*
 * Flag values for bcb_flags.
 */
#define BCB_PHYSCONTIG	(1 << 0)	/* requires physically-contiguous
					 * buffers (if not BA_SCGTH) */
#define BCB_ONE_PIECE	(1 << 1)	/* whole job must be done in one
					 * piece (fail if not possible) */
#define BCB_EXACT_SIZE	(1 << 2)	/* fail jobs with unaligned offset
					 * or size */
#define BCB_SYNCHRONOUS	(1 << 3)	/* do not send multiple simultaneous
					 * jobs (also fail any B_ASYNC) */
#define BCB_NOMEMWAIT	(1 << 4)	/* driver never waits for memory
					 * allocations (directly or
					 * indirectly) */

/*
 * Flags for getblk()
 */
#define	BG_NOHIT	0x01  /* Caller doesn't want from cache-just checking */
#define BG_NOMISS	0x02  /* Caller doesn't want if not in cache */
#define BG_NOFREESPACE	0x04  /* Don't block if not in cache and
				 no free space			     */
#define BG_NOWAIT	0x08  /* Don't block if in cache and in use  */
#define BG_NOFLUSH	0x10  /* Don't flush if free with B_DELWRI set */

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

#if !defined(_DDI)

#define	NOMEMWAIT() (IS_PAGEOUT() || (u.u_flags & U_CRITICAL_MEM))

#ifdef _PAGEIO_HIST

/*
 * Pageio logging facility macros and function definitions.
 */

#define PAGEIO_LOG(bp, msg)		\
			pageio_log(bp, msg, __LINE__, __FILE__)
#define PAGEIO_LOG_PP(bp, pp, flags, msg)	\
			pageio_log_pp(bp, pp, flags, msg, __LINE__, __FILE__)
#define PAGEIO_LOG_PAGES(pp, flags, msg)	\
			pageio_log_pages(pp, flags, msg, __LINE__, __FILE__)

struct page;
extern void pageio_log(buf_t *, char *, int, char *);
extern void pageio_log_pp(buf_t *, struct page *, uint_t, char *, int, char *);
extern void pageio_log_pages(struct page *, uint_t, char *, int, char *);

#else

#define PAGEIO_LOG(bp, msg)			((void)0)
#define PAGEIO_LOG_PP(bp, msg, flags, pp)	((void)0)
#define PAGEIO_LOG_PAGES(pp, flags, msg)	((void)0)

#endif

extern struct hbuf hbuf[];	/* hash table */

/*
 * Insq/Remq for the buffer hash lists.
 */
#define	bhashself(bp) { \
	(bp)->b_forw = (bp)->b_back = (bp); \
}
#define	bremhash(bp) { \
	(bp)->b_back->b_forw = (bp)->b_forw; \
	(bp)->b_forw->b_back = (bp)->b_back; \
	 bhashself(bp); \
}
#define	binshash(bp, dp) { \
	(bp)->b_forw = (dp)->b_forw; \
	(bp)->b_back = (dp); \
	(dp)->b_forw->b_back = (bp); \
	(dp)->b_forw = (bp); \
}

/*
 * Insq/Remq for the buffer free lists.
 */
#define	bremfree(bp) { \
	(bp)->av_back->av_forw = (bp)->av_forw; \
	(bp)->av_forw->av_back = (bp)->av_back; \
	(bp)->av_forw = (bp)->av_back = (bp); \
}
/*
 * Insert buffer bp after dp.
 */
#define	bfrontfree(bp, dp) { \
	(dp)->av_forw->av_back = (bp); \
	(bp)->av_forw = (dp)->av_forw; \
	(dp)->av_forw = (bp); \
	(bp)->av_back = (dp); \
}

/*
 * Insert buffer bp in front of dp.
 */

#define	bbackfree(bp, dp) { \
	(dp)->av_back->av_forw = (bp); \
	(bp)->av_back = (dp)->av_back; \
	(dp)->av_back = (bp); \
	(bp)->av_forw = (dp); \
}

/*
 * Initialize marker buffer for free list traversing.
 */ 

#define init_bmarker(bp) { \
 	(bp)->b_flags	= B_MARKER; \
}

extern lkinfo_t	buf_avail_lkinfo;

#define BUF_INIT(bp) { \
	(bp)->b_flags     = B_KERNBUF; \
	(bp)->b_edev      = (dev_t)NODEV; \
	(bp)->b_un.b_addr = NULL; \
	(bp)->b_bcount    = 0; \
	EVENT_INIT(&(bp)->b_iowait); \
	SLEEP_INIT(&(bp)->b_avail, (uchar_t) 0, &buf_avail_lkinfo, KM_SLEEP); \
	(bp)->av_forw = (bp)->av_back = (bp); \
	bhashself((bp)); \
}

#define BUF_DEINIT(bp) { \
	SLEEP_DEINIT(&(bp)->b_avail); \
}

/*
 * bufinval(buf_t *bp)
 *	Invalidate a buffer.
 *
 * Calling/Exit State:
 *	The calling LWP must own the buffer.
 *
 * Description
 *	The buffer is invalidated by setting B_STALE. The calling LWP
 *	must release the buffer.
 *
 */
#define bufinval(bp)	((bp)->b_flags |= B_STALE)

/*
 *	Fast access to buffers in cache by hashing.
 */

#define bhash(dev, blk) ((buf_t *)&hbuf[((int)(dev) + ((int)(blk) >> 4)) % \
					 v.v_hbuf])

/*
 * Unlink a buffer from the available list and obtain it's allocation
 * lock. There shouldn't be *any* contention on this lock when doing
 * notavail - the decision was made while holding the lists lock.
 * (internal interface)
 */
#define notavail(bp)				\
{						\
	ASSERT(!SLEEP_LOCKBLKD(&(bp)->b_avail));\
	bremfree(bp);				\
	ASSERT(SLEEP_LOCKAVAIL(&(bp)->b_avail));\
	(void) SLEEP_TRYLOCK(&(bp)->b_avail);	\
	(bp)->b_flags |= B_BUSY;		\
}

#define bawrite(bp)			\
{					\
	(bp)->b_flags |= B_ASYNC;	\
	bwrite(bp);			\
}

extern buf_t *bclnlist;

#endif /* !_DDI */

#if defined(__STDC__)

struct page;

void binit(void);
buf_t *bread(dev_t, daddr_t, long);
buf_t *breada(dev_t, daddr_t, daddr_t, long);
buf_t *pbread(dev_t, daddr_t, long);
buf_t *blookup(dev_t, daddr_t, long);
int bwrite(buf_t *);
void bdwrite(buf_t *);
void btwrite(buf_t *, clock_t);
buf_t *getfreeblk(long, int, int, pl_t *);
buf_t *getblk(dev_t, daddr_t, long, int);
buf_t *pgetblk(dev_t, daddr_t, long);
void bflush(dev_t);
void blkflush(dev_t, daddr_t);
void bdwait(dev_t);
void pgwait(dev_t);
void basyndone(buf_t *);
void binval(dev_t);
buf_t *pageio_setup(struct page *, uint_t, uint_t, int);
void pageio_done(buf_t *);
void basyncdone(buf_t *);
void cleanup(void);
void biodone(buf_t *);
void bdelaydone(buf_t *);
int biowait(buf_t *);
boolean_t biowait_sig(buf_t *);
void brelse(buf_t *);
void bp_mapin(buf_t *);
void bp_mapout(buf_t *);
void clrbuf(buf_t *);
void freerbuf(buf_t *);
buf_t *geteblk(void);
buf_t *getrbuf(int);
buf_t *ngeteblk(long);
bcb_t *bcb_alloc(int);
void bcb_free(bcb_t *);
void buf_breakup(void (*strat)(buf_t *), buf_t *bp, const bcb_t *bcbp);

#else /* __STDC__ */

/* Only functions used by DDI/DKI drivers need to be declared non-ANSI */

void biodone();
void bdelaydone();
int biowait();
boolean_t biowait_sig();
void brelse();
void bp_mapin();
void bp_mapout();
void clrbuf();
void freerbuf();
buf_t *geteblk();
buf_t *getrbuf();
buf_t *ngeteblk();
bcb_t *bcb_alloc();
void bcb_free();
void buf_breakup();

#endif /* __STDC__ */ 

#if !defined(_DDI)

/*
 * Reset a buffer after completed I/O so it can be used again.
 */
#define bioreset(bp) \
	(ASSERT((bp)->b_flags & B_KERNBUF), \
	 ASSERT(!SLEEP_LOCKAVAIL(&(bp)->b_avail)), \
	 ASSERT((bp)->b_flags & B_BUSY), \
	 ((bp)->b_flags &= ~(B_DONE|B_ERROR)), \
	 ((bp)->b_error = 0))

/*
 * Pick up the device's error number and pass it to the user;
 * if there is an error but the number is 0 set a generalized code.
 */
#define geterror(bp) \
	(!((bp)->b_flags & B_ERROR) ? 0 : \
	 ((bp)->b_error ? (bp)->b_error : EIO))

/*
 * Set or clear the error flag and error field in a buffer header.
 */
#ifndef lint
#define bioerror(bp, errno) \
	(((bp)->b_error = (errno)), \
	 ((errno) ? \
		((bp)->b_flags |= B_ERROR) : \
		((bp)->b_flags &= ~B_ERROR)))
#else
#define bioerror(bp, errno) ((bp)->b_error = (errno))
#endif

#endif /* !_DDI */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_BUF_H */
