/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* $XConsortium: FSQGlyphs.c,v 1.4 92/05/26 17:27:39 gildea Exp $ */
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

int
FSQueryXBitmaps8(svr, fid, format, range_type, str, str_len, offsets, glyphdata)
    FSServer   *svr;
    Font        fid;
    fsBitmapFormat format;
    Bool        range_type;
    unsigned char *str;
    unsigned long str_len;
    fsOffset  **offsets;
    unsigned char **glyphdata;
{
    fsQueryXBitmaps8Req *req;
    fsQueryXBitmaps8Reply reply;
    fsOffset   *offs;
    unsigned char *gd;
    int         left;

    GetReq(QueryXBitmaps8, req);
    req->fid = fid;
    req->range = range_type;
    req->format = format;
    req->num_ranges = str_len;
    req->length += (str_len + 3) >> 2;
    _FSSend(svr, (char *) str, str_len);

    /* get back the info */
    if (!_FSReply(svr, (fsReply *) & reply,
     (sizeof(fsQueryXBitmaps8Reply) - sizeof(fsGenericReply)) >> 2, fsFalse))
	return FSBadAlloc;

    offs = (fsOffset *) FSmalloc(sizeof(fsOffset) * reply.num_chars);
    *offsets = offs;
    if (!offs)
	return FSBadAlloc;
    left = (reply.length << 2) - sizeof(fsQueryXBitmaps8Reply)
	- (sizeof(fsOffset) * reply.num_chars);
    gd = (unsigned char *) FSmalloc(left);
    *glyphdata = gd;
    if (!gd) {
	FSfree((char *) offs);
	return FSBadAlloc;
    }
    _FSReadPad(svr, (char *) offs, (sizeof(fsOffset) * reply.num_chars));
    _FSReadPad(svr, (char *) gd, left);

    SyncHandle();
    return FSSuccess;
}

int
FSQueryXBitmaps16(svr, fid, format, range_type, str, str_len,
		  offsets, glyphdata)
    FSServer   *svr;
    Font        fid;
    fsBitmapFormat format;
    Bool        range_type;
    fsChar2b   *str;
    unsigned long str_len;
    fsOffset  **offsets;
    unsigned char **glyphdata;
{
    fsQueryXBitmaps16Req *req;
    fsQueryXBitmaps16Reply reply;
    fsOffset   *offs;
    unsigned char *gd;
    int         left;

    GetReq(QueryXBitmaps16, req);
    req->fid = fid;
    req->range = range_type;
    req->format = format;
    req->num_ranges = str_len;
    req->length += ((str_len * sizeof(fsChar2b)) + 3) >> 2;
    if (FSProtocolVersion(svr) == 1)
    {
	int i;
	fsChar2b_version1 *swapped_str;

	swapped_str = (fsChar2b_version1 *)
	    FSmalloc(sizeof(fsChar2b_version1) * str_len);
	if (!swapped_str)
	    return FSBadAlloc;
	for (i = 0; i < str_len; i++) {
	    swapped_str[i].low = str[i].low;
	    swapped_str[i].high = str[i].high;
	}
	_FSSend(svr, (char *)swapped_str, (str_len*sizeof(fsChar2b_version1)));
	FSfree(swapped_str);
    } else
	_FSSend(svr, (char *) str, (str_len * sizeof(fsChar2b)));

    /* get back the info */
    if (!_FSReply(svr, (fsReply *) & reply,
	      (sizeof(fsQueryXBitmaps16Reply) - sizeof(fsGenericReply)) >> 2,
		  fsFalse))
	return FSBadAlloc;

    offs = (fsOffset *) FSmalloc(sizeof(fsOffset) * reply.num_chars);
    *offsets = offs;
    if (!offs)
	return FSBadAlloc;
    left = (reply.length << 2) - sizeof(fsQueryXBitmaps16Reply)
	- (sizeof(fsOffset) * reply.num_chars);
    gd = (unsigned char *) FSmalloc(left);
    *glyphdata = gd;
    if (!gd) {
	FSfree((char *) offs);
	return FSBadAlloc;
    }
    _FSReadPad(svr, (char *) offs, (sizeof(fsOffset) * reply.num_chars));
    _FSReadPad(svr, (char *) gd, left);

    SyncHandle();
    return FSSuccess;
#ident	"@(#)r5fonts:lib/fs/FSQGlyphs.c	1.2"
}
