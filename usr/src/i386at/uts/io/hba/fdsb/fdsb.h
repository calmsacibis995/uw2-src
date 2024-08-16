/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-pdi:io/hba/fdsb/fdsb.h	1.1"
/*
 * fdsb.h       Copyright Future Domain Corp., 1993-94. All rights reserved.
 */
typedef unsigned char  osd_uchar;
typedef unsigned short osd_ushort;
typedef unsigned long  osd_ulong;

#define FDC_MAX_ADAPTERS  4
#define FDC_MAX_INQ_BYTES 36

typedef struct fdc_idev {
    osd_uchar       fdd_inq_data[FDC_MAX_INQ_BYTES + 1];
    osd_uchar       fdd_len;            /* Length required for a match */
    osd_ushort      fdd_flags;          /* "Special instruction" flags */
    osd_uchar       fdx_sdtr_state;     /* State of SDTR negotiation */
    osd_uchar       fdx_offset;         /* Current negotiated ack offset */
    osd_ushort      fdx_period;         /* Current negotiated ack period */
    osd_uchar       fdx_sync_ctrl_reg;  /* Current register bitmask */
    osd_uchar       fdx_reserved1[3];   /* Reserved for future use */
} fdc_idev_t;
