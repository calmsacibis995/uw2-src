/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/dlpi_cpq/pnt/pnt.cf/Space.c	1.4"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/stream.h>
#include <sys/dlpi_ether.h>
#include <config.h>


#define	NSAPS		8
#define MAXMULTI	8
#define INETSTATS	1
#define STREAMS_LOG	0
#define IFNAME		"pnt"

#define	PNT_TX_BUF	16
#define	PNT_RX_BUF	16

int pnt_MaxStreams	= NSAPS;
int pntstrlog		= STREAMS_LOG;
char *pnt_ifname	= IFNAME;
int pnt_nsaps		= NSAPS;
int pnt_cmajors		= PNT_CMAJORS;	/* # of possible major #'s */
int pnt_cmajor_0	= PNT_CMAJOR_0;	/* first major # in range */
short pnt_tx_buf	= PNT_TX_BUF;
short pnt_rx_buf	= PNT_RX_BUF;

struct pnt_ConfigStruct {
	short index;
	short minors;
	short vec;
	ushort iobase;
	short dma;
	ushort junk;
	short tx_buffers;
	short rx_buffers;
	long  bus_scan;
	long  led0;
	long  led1;
	long  led2;
	long  led3;
	long  dmarotate;
	long  tp;
	long  fdup;
};

struct pnt_ConfigStruct pnt_ConfigArray[4] = {
    {
	0,
	NSAPS,
	0,
	0,
	-1,
        5,
	PNT_TX_BUF,
	PNT_RX_BUF,
	0,      /* bus to scan, MUST be set to 5 (PCI1) on PCI 1 machines */
	0,      /* led 0 */
	0,      /* led 1 */
	0,      /* led 2 */ 
	0,      /* led 3 */
	0,      /* dmarotate */
	0,      /* TP */
	0      /* full duplex */
    }
    ,
    {
	1,
	NSAPS,
	0,
	0,
	-1,
        5,
	PNT_TX_BUF,
	PNT_RX_BUF,
	0,      /* bus to scan, MUST be set to 5 (PCI1) on PCI 1 machines */
	0,      /* led 0 */
	0,      /* led 1 */
	0,      /* led 2 */ 
	0,      /* led 3 */
	0,      /* dmarotate */
	0,      /* TP */
	0      /* full duplex */
    }
    ,
    {
	2,
	NSAPS,
	0,
	0,
	-1,
        5,
	PNT_TX_BUF,
	PNT_RX_BUF,
	0,      /* bus to scan, MUST be set to 5 (PCI1) on PCI 1 machines */
	0,      /* led 0 */
	0,      /* led 1 */
	0,      /* led 2 */ 
	0,      /* led 3 */
	0,      /* dmarotate */
	0,      /* TP */
	0      /* full duplex */
    }
    ,
    {
	3,
	NSAPS,
	0,
	0,
	-1,
        5,
	PNT_TX_BUF,
	PNT_RX_BUF,
	0,      /* bus to scan, MUST be set to 5 (PCI1) on PCI 1 machines */
	0,      /* led 0 */
	0,      /* led 1 */
	0,      /* led 2 */ 
	0,      /* led 3 */
	0,      /* dmarotate */
	0,      /* TP */
	0      /* full duplex */
    }
};

