/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* $XConsortium: FSSync.c,v 1.4 92/05/26 17:14:57 gildea Exp $ */
/*
 * Copyright 1990 Network Computing Devices;
 * Portions Copyright 1987 by Digital Equipment Corporation and the
 * Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Network Computing Devices, Digital or
 * M.I.T. not be used in advertising or publicity pertaining to distribution
 * of the software without specific, written prior permission.
 *
 * NETWORK COMPUTING DEVICES, DIGITAL AND M.I.T. DISCLAIM ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL NETWORK COMPUTING DEVICES,
 * DIGITAL OR M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 */

#include "FSlibint.h"

extern _FSQEvent *_FSqfree;

/* synchronize with errors and events */

FSSync(svr, discard)
    FSServer     *svr;
    Bool        discard;
{
    fsListExtensionsReply rep;
    fsReq      *req;

    GetEmptyReq(ListExtensions, req);
    (void) _FSReply(svr, (fsReply *) & rep, 0, fsTrue);

    if (discard && svr->head) {
	((_FSQEvent *) svr->tail)->next = _FSqfree;
	_FSqfree = (_FSQEvent *) svr->head;
	svr->head = svr->tail = NULL;
	svr->qlen = 0;
    }
#ident	"@(#)r5fonts:lib/fs/FSSync.c	1.2"
}
