/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/npsd/dl.h	1.7"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: dl.h,v 1.4 1994/08/29 16:38:16 vtag Exp $"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

/*
** Strings for Constructing Tokens
*/

#define PPA_TOKEN			"_ppa"
#define FRAME_TYPE_TOKEN	"_frame_type"
#define IF_NAME_TOKEN		"_if_name"

/*
 * Stuff for DLPIBind() Routine
 */
#define	NPSD_ADDR_PAD				16
#define	NPSD_BIND_ACK_BUF_SIZE		(sizeof(dl_bind_ack_t)+NPSD_ADDR_PAD)
#define	NPSD_SUBS_BIND_BUF_SIZE		(sizeof(dl_subs_bind_req_t)+NPSD_ADDR_PAD)

#define	NOVELL_SAP		0x8137
#define RAW_8023		0xE0
#define DSAP_8022		0xE0
#define DSAP_8022_SNAP	0xAA
#define DSAP_8025		0xE0
#ifdef NW_UP
#define DSAP_8025_SNAP	0xAA
#else
#define DSAP_8025_SNAP	0x8137
#endif

#define FRAME_ETHERNET2	0x8137
#define FRAME_8022_SNAP	0x8138
#define FRAME_8022		0x8022
#define FRAME_8023		0x8023
#define FRAME_8025_SNAP	0x8238
#define FRAME_8025		0x8025

/*
 * Structure for map between frame type keyword and function.
 */
typedef struct {
    int	 frame;
    char *name;
} f_functionMap_t;

typedef struct {
	uint32	snapOrg;
	uint32	snapType;
} snapHdr_t;

#define	NOVELL_SNAP_ORG			0
#define	NOVELL_SNAP_TYPE		NOVELL_SAP

#define	ETALK_SNAP_ORG			0x080007
#define	ETALK_SNAP_TYPE			0x809b

#define	AARP_SNAP_ORG			0
#define	AARP_SNAP_TYPE			0x80f3
