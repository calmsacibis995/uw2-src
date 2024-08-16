/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_DLPI_CPQ_CET_SR_NFLXSR_H	/* wrapper symbol for kernel use */
#define _IO_DLPI_CPQ_CET_SR_NFLXSR_H	/* subject to change without notice */

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/



#ident	"@(#)kern-i386at:io/dlpi_cpq/cet/sr/nflxsr.h	1.2"
#ident	"$Header: $"

#ifdef	ESMP
#ifdef	_KERNEL_HEADERS
#include <util/ksynch.h>	/* REQUIRED */
#else
#include <sys/ksynch.h>		/* REQUIRED */
#endif
#endif	/* ESMP */

#define		MAC_ADDR_LEN		6
#define		MAX_ROUTE_SIZE		18
#ifndef	LLC_NULL_SAP
#define		LLC_NULL_SAP		0
#endif
#define		IP_SAP			0x800
#define		ARP_SAP			0x806

#define		NFLXSR_NULL_SAP_SIZE	1
#define		INIT_ROUTE_HDR_SIZE	0x2

#define		SOURCE_ROUTE_BIT	0x80
#define		SOURCE_ROUTE_DIR_BIT	0x80
#define		SINGLE_ROUTE_BCAST	0xE0
#define		ALL_ROUTE_BCAST		0xA0
#define		LARGEST_FRAME_SIZE	0x70 	/* This represents possible 
					         * largest frame size (18K)
						 * the token bridge can
						 * handle 
						 */

#define TEST_DATA_SIZE	(DL_TEST_REQ_SIZE + MAX_ROUTE_SIZE + MAC_ADDR_LEN +\
						NFLXSR_NULL_SAP_SIZE * 2)

#define		MAX_TAB_SIZE		13
#define		MAX_WAIT_TAB_SIZE	10

#define		LOCAL_REPLY_WAIT	3
#define		REMOTE_REPLY_WAIT	6

#define		NFLXSR_PAGE_SIZE		1024
#define		NFLXSR_TIMEOUT		5

#define		NFLXSR_HASH(X)	(((unsigned char)(*(X + 5))) % MAX_TAB_SIZE)
#define		MAXMINORS	32

#define		COPY_MACADDR(y, x)	\
	(x[0] = y[0], x[1] = y[1], x[2] = y[2], \
	 x[3] = y[3], x[4] = y[4], x[5] = y[5])

#define		SAME_MACADDR(x, y)	\
	(x[0] == y[0] && x[1] == y[1] && x[2] == y[2] && \
	 x[3] == y[3] && x[4] == y[4] && x[5] == y[5])

#define		BROADCAST(x)	\
	(x[0] == 0xff && x[1] == 0xff && x[2] == 0xff && \
	 x[3] == 0xff && x[4] == 0xff && x[5] == 0xff)

#define		HIWAT		24576
#define		LOWAT		2048
#define		MAXPKT		1526

#ifdef	ESMP
#define		NFLXSRTAB_LOCK_HIER		2
#define		NFLXSRWAIT_LOCK_HIER		2
#define		NFLXSRDEV_LOCK_HIER		2
#define		NFLXSR_LOCK_HIER		3

extern	lock_t	*nflxsrtab_lock;
extern	lock_t	*nflxsrwait_lock;
extern	lock_t	*nflxsrdev_lock;

#define	NFLXSR_LOCK(X, Y)		(LOCK((X), (Y)))
#define	NFLXSR_UNLOCK(X, Y)		(UNLOCK((X), (Y)))
#else
#define	NFLXSR_LOCK(X, Y)		(splstr())
#define	NFLXSR_UNLOCK(X, Y)		(splx(Y))
#endif

typedef struct nflxsr_basic_route_info {
	unsigned char 	nflxsr_macaddr[MAC_ADDR_LEN];	/*The dest mac address*/
	ushort		nflxsr_route_size;			/*Size of route field*/
	unsigned char 	nflxsr_route[MAX_ROUTE_SIZE];	/*Src route info */
} nflxsr_basic_route_info_t;

#define 	BASIC_ROUTE_INFO_SIZE		sizeof(nflxsr_basic_route_info_t)
#define		NFLXSR_DUMP_ROUTE_TABLE		0xFE /*Only temp */

typedef struct nflxsr_elem {
	unsigned char 	nflxsr_macaddr[MAC_ADDR_LEN];	/*The dest mac address*/
	ushort		nflxsr_route_size;			/*Size of route field*/
	unsigned char 	nflxsr_route[MAX_ROUTE_SIZE];	/*Src route info */
	unsigned char 	nflxsr_netno;			/*Direction of route */
	unsigned char 	nflxsr_avail;			/*Availibility of slot*/
	struct nflxsr_elem	*nflxsr_next;
	caddr_t		nflxsr_memp;
	ulong		nflxsr_time;
#ifdef	ESMP
	lock_t		*nflxsr_lock;
#endif
} nflxsr_elem_t;

#define	MAX_LIST_SIZE	\
	((NFLXSR_PAGE_SIZE - sizeof(struct nflxsr_mem *) - sizeof(ulong)) / sizeof(nflxsr_elem_t))
typedef struct nflxsr_mem {
	struct nflxsr_mem * nflxsr_hdr_next;
	ulong	nflxsr_num_avail;
	nflxsr_elem_t nflxsr_elem_list[MAX_LIST_SIZE];
} nflxsr_mem_t;

#define		NFLXSR_UNUSED			0x0
#define		NFLXSR_WAITING_FOR_LOCAL_REPLY	0x1
#define		NFLXSR_WAITING_FOR_REMOTE_REPLY	0x2
#define		NFLXSR_ROUTE_KNOWN			0x4
#define		NFLXSR_ROUTE_UNKNOWN		0x8

typedef struct nflxsr_wait_elem {
	unsigned char 	nflxsr_wait_macaddr[MAC_ADDR_LEN];
	unsigned char 	nflxsr_state;
	ushort		nflxsr_timer_val;
	ushort		nflxsr_route_size;
	unsigned char 	nflxsr_route[MAX_ROUTE_SIZE];
	queue_t 	*nflxsr_streamq;
	queue_t 	nflxsr_waitq;
} nflxsr_wait_elem_t;


typedef struct nflxsrdev {
	unsigned char 	macaddr[MAC_ADDR_LEN];
	queue_t *nflxsr_qptr;
	ulong	std_addr_length;
	ushort nflxsr_sap;
#ifdef	ESMP
	/*
	 * nflxsr_bypass:
	 *	-A non-zero value indicates that Source Routing functionality
	 *	 is to be bypassed (i.e. M_PROTO messages are just passed on
	 *	 through without being processed).
	 *	-A zero value indicates that Source Routing functionality
	 *	 is NOT to be bypassed (i.e. M_PROTO messages are processed).
	 */
	atomic_int_t nflxsr_bypass;
#endif
}nflxsrdev_t;

#endif	/* _IO_DLPI_CPQ_CET_SR_NFLXSR_H */
