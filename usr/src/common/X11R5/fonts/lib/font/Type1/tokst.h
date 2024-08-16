/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fonts:lib/font/Type1/tokst.h	1.1"

/* $XConsortium: tokst.h,v 1.2 91/10/10 11:20:00 rws Exp $ */
/* Copyright International Business Machines,Corp. 1991
 * All Rights Reserved
 *
 * License to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is
 * hereby granted, provided that the above copyright notice
 * appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation,
 * and that the name of IBM not be used in advertising or
 * publicity pertaining to distribution of the software without
 * specific, written prior permission.
 *
 * IBM PROVIDES THIS SOFTWARE "AS IS", WITHOUT ANY WARRANTIES
 * OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT
 * LIMITED TO ANY IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS.  THE ENTIRE RISK AS TO THE QUALITY AND
 * PERFORMANCE OF THE SOFTWARE, INCLUDING ANY DUTY TO SUPPORT
 * OR MAINTAIN, BELONGS TO THE LICENSEE.  SHOULD ANY PORTION OF
 * THE SOFTWARE PROVE DEFECTIVE, THE LICENSEE (NOT IBM) ASSUMES
 * THE ENTIRE COST OF ALL SERVICING, REPAIR AND CORRECTION.  IN
 * NO EVENT SHALL IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */
/* -------------------------------------- */
/* --- MACHINE GENERATED, DO NOT EDIT --- */
/* -------------------------------------- */
 
#ifndef TOKST
#define TOKST 1
 
/*
 * State Index Tables --
 *
 *   These tables map the input character to the
 *   proper entry in the Class Action Table.
 *   There is one table for each state.
 *
 */
#define s0 (si0+2)
static unsigned char si0[258] = { 0x10,0x11,
 0x02,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x02,0x02,0x0F,0x0F,0x02,0x0F,0x0F,
 0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
 0x02,0x0F,0x0F,0x0F,0x0F,0x03,0x0F,0x0F,0x05,0x0B,0x0F,0x0D,0x0F,0x0D,0x0E,0x04,
 0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x0F,0x0F,0x08,0x0F,0x0C,0x0F,
 0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0x0F,0x0A,0x0F,0x0F,
 0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x0F,0x09,0x0F,0x0F,
 0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
 0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
 0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
 0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
 0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
 0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
 0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
 0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F
};
 
#define s1 (si1+2)
static unsigned char si1[258] = { 0x14,0x15,
 0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
 0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
 0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x12,
 0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
 0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
 0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
 0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
 0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
 0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
 0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
 0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
 0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
 0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
 0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
 0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,
 0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13,0x13
};
 
#define s2 (si2+2)
static unsigned char si2[258] = { 0x1B,0x1C,
 0x16,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x16,0x16,0x1A,0x1A,0x16,0x1A,0x1A,
 0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
 0x16,0x1A,0x1A,0x1A,0x1A,0x17,0x1A,0x1A,0x17,0x17,0x1A,0x1A,0x1A,0x1A,0x19,0x17,
 0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x1A,0x1A,0x17,0x1A,0x17,0x1A,
 0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
 0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x17,0x1A,0x17,0x1A,0x1A,
 0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
 0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x17,0x1A,0x17,0x1A,0x1A,
 0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
 0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
 0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
 0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
 0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
 0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
 0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,
 0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A,0x1A
};
 
#define s3 (si3+2)
static unsigned char si3[258] = { 0x23,0x24,
 0x1D,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x1D,0x1D,0x22,0x22,0x1D,0x22,0x22,
 0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
 0x1D,0x22,0x22,0x20,0x22,0x1E,0x22,0x22,0x1E,0x1E,0x22,0x22,0x22,0x22,0x1F,0x1E,
 0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x1E,0x22,0x1E,0x22,
 0x22,0x22,0x22,0x22,0x22,0x21,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
 0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x1E,0x22,0x1E,0x22,0x22,
 0x22,0x22,0x22,0x22,0x22,0x21,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
 0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x1E,0x22,0x1E,0x22,0x22,
 0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
 0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
 0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
 0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
 0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
 0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
 0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,
 0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22
};
 
#define s4 (si4+2)
static unsigned char si4[258] = { 0x29,0x2A,
 0x25,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x25,0x25,0x28,0x28,0x25,0x28,0x28,
 0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,
 0x25,0x28,0x28,0x28,0x28,0x26,0x28,0x28,0x26,0x26,0x28,0x28,0x28,0x28,0x28,0x26,
 0x27,0x27,0x27,0x27,0x27,0x27,0x27,0x27,0x27,0x27,0x28,0x28,0x26,0x28,0x26,0x28,
 0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,
 0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x26,0x28,0x26,0x28,0x28,
 0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,
 0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x26,0x28,0x26,0x28,0x28,
 0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,
 0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,
 0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,
 0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,
 0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,
 0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,
 0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,
 0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28,0x28
};
 
#define s5 (si5+2)
static unsigned char si5[258] = { 0x30,0x31,
 0x2B,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2B,0x2B,0x2F,0x2F,0x2B,0x2F,0x2F,
 0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,
 0x2B,0x2F,0x2F,0x2F,0x2F,0x2C,0x2F,0x2F,0x2C,0x2C,0x2F,0x2F,0x2F,0x2F,0x2F,0x2C,
 0x2E,0x2E,0x2E,0x2E,0x2E,0x2E,0x2E,0x2E,0x2E,0x2E,0x2F,0x2F,0x2C,0x2F,0x2C,0x2F,
 0x2F,0x2F,0x2F,0x2F,0x2F,0x2D,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,
 0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2C,0x2F,0x2C,0x2F,0x2F,
 0x2F,0x2F,0x2F,0x2F,0x2F,0x2D,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,
 0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2C,0x2F,0x2C,0x2F,0x2F,
 0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,
 0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,
 0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,
 0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,
 0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,
 0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,
 0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,
 0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F,0x2F
};
 
#define s6 (si6+2)
static unsigned char si6[258] = { 0x36,0x37,
 0x32,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x32,0x32,0x35,0x35,0x32,0x35,0x35,
 0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
 0x32,0x35,0x35,0x35,0x35,0x33,0x35,0x35,0x33,0x33,0x35,0x35,0x35,0x35,0x35,0x33,
 0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x33,0x35,0x33,0x35,
 0x35,0x35,0x35,0x35,0x35,0x34,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
 0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x33,0x35,0x33,0x35,0x35,
 0x35,0x35,0x35,0x35,0x35,0x34,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
 0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x33,0x35,0x33,0x35,0x35,
 0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
 0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
 0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
 0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
 0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
 0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
 0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,
 0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35,0x35
};
 
#define s7 (si7+2)
static unsigned char si7[258] = { 0x3D,0x3E,
 0x38,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x38,0x38,0x3C,0x3C,0x38,0x3C,0x3C,
 0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,
 0x38,0x3C,0x3C,0x3C,0x3C,0x39,0x3C,0x3C,0x39,0x39,0x3C,0x3A,0x3C,0x3A,0x3C,0x39,
 0x3B,0x3B,0x3B,0x3B,0x3B,0x3B,0x3B,0x3B,0x3B,0x3B,0x3C,0x3C,0x39,0x3C,0x39,0x3C,
 0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,
 0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x39,0x3C,0x39,0x3C,0x3C,
 0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,
 0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x39,0x3C,0x39,0x3C,0x3C,
 0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,
 0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,
 0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,
 0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,
 0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,
 0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,
 0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,
 0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C,0x3C
};
 
#define s8 (si8+2)
static unsigned char si8[258] = { 0x43,0x44,
 0x3F,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x3F,0x3F,0x42,0x42,0x3F,0x42,0x42,
 0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,
 0x3F,0x42,0x42,0x42,0x42,0x40,0x42,0x42,0x40,0x40,0x42,0x42,0x42,0x42,0x42,0x40,
 0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x42,0x42,0x40,0x42,0x40,0x42,
 0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,
 0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x40,0x42,0x40,0x42,0x42,
 0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,
 0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x40,0x42,0x40,0x42,0x42,
 0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,
 0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,
 0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,
 0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,
 0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,
 0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,
 0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,
 0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42,0x42
};
 
#define s9 (si9+2)
static unsigned char si9[258] = { 0x48,0x49,
 0x45,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x45,0x45,0x47,0x47,0x45,0x47,0x47,
 0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,
 0x45,0x47,0x47,0x47,0x47,0x46,0x47,0x47,0x46,0x46,0x47,0x47,0x47,0x47,0x47,0x46,
 0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x46,0x47,0x46,0x47,
 0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,
 0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x46,0x47,0x46,0x47,0x47,
 0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,
 0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x46,0x47,0x46,0x47,0x47,
 0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,
 0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,
 0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,
 0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,
 0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,
 0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,
 0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,
 0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47
};
 
#define s10 (si10+2)
static unsigned char si10[258] = { 0x4E,0x4F,
 0x4A,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4A,0x4A,0x4D,0x4D,0x4A,0x4D,0x4D,
 0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,
 0x4A,0x4D,0x4D,0x4D,0x4D,0x4B,0x4D,0x4D,0x4B,0x4B,0x4D,0x4D,0x4D,0x4D,0x4D,0x4B,
 0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4D,0x4D,0x4B,0x4D,0x4B,0x4D,
 0x4D,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,
 0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4B,0x4D,0x4B,0x4D,0x4D,
 0x4D,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,
 0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4C,0x4B,0x4D,0x4B,0x4D,0x4D,
 0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,
 0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,
 0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,
 0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,
 0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,
 0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,
 0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,
 0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D,0x4D
};
 
#define s11 (si11+2)
static unsigned char si11[258] = { 0x53,0x54,
 0x50,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x50,0x50,0x52,0x52,0x50,0x52,0x52,
 0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,
 0x50,0x52,0x52,0x52,0x52,0x51,0x52,0x52,0x51,0x51,0x52,0x52,0x52,0x52,0x52,0x51,
 0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x51,0x52,0x51,0x52,
 0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,
 0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x51,0x52,0x51,0x52,0x52,
 0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,
 0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x51,0x52,0x51,0x52,0x52,
 0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,
 0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,
 0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,
 0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,
 0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,
 0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,
 0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,
 0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52,0x52
};
 
/*
 * Class Action Table --
 *
 *   The entries in the Class Action Table indicate the
 *   action routine to be called, and the next state to
 *   enter, for each relevant character class in each.
 *   state. There are several entries for each state.
 *
 */
static int AAH_NAME();
static int BREAK_SIGNAL();
static int HEX_STRING();
static int IMMED_NAME();
static int INTEGER();
static int LEFT_BRACE();
static int LEFT_BRACKET();
static int LITERAL_NAME();
static int NAME();
static int NO_TOKEN();
static int OOPS_NAME();
static int RADIX_NUMBER();
static int REAL();
static int RIGHT_ANGLE();
static int RIGHT_BRACE();
static int RIGHT_BRACKET();
static int RIGHT_PAREN();
static int STRING();
static int add_1st_decpt();
static int add_1st_digits();
static int add_char();
static int add_decpt();
static int add_digits();
static int add_e_sign();
static int add_exponent();
static int add_fraction();
static int add_r_digits();
static int add_radix();
static int add_sign();
static int next_char();
static int skip_comment();
static int skip_space();
 
static struct cat {
  int (*actionRoutineP)();
  unsigned char *nextStateP;
} classActionTable[] = {
 
  /* s0:  Classify initial character */
  /* 00 ALPHA          */ {NAME,           s0},  /* executable name */
  /* 01 DIGIT          */ {add_1st_digits, s3},  /* number? */
  /* 02 WHITE_SPACE    */ {skip_space,     s0},  /* skip white space */
  /* 03 PERCENT        */ {skip_comment,   s0},  /* comment? */
  /* 04 SLASH          */ {next_char,      s1},  /* literal or imm name */
  /* 05 LEFT_PAREN     */ {STRING,         s0},  /* string */
  /* 06 LEFT_BRACE     */ {LEFT_BRACE,     s0},  /* begin procedure body */
  /* 07 LEFT_BRACKET   */ {LEFT_BRACKET,   s0},  /* begin array */
  /* 08 LEFT_ANGLE     */ {HEX_STRING,     s0},  /* hex string? */
  /* 09 RIGHT_BRACE    */ {RIGHT_BRACE,    s0},  /* end procedure body */
  /* 0A RIGHT_BRACKET  */ {RIGHT_BRACKET,  s0},  /* end array */
  /* 0B RIGHT_PAREN    */ {RIGHT_PAREN,    s0},  /* unmatched right paren */
  /* 0C RIGHT_ANGLE    */ {RIGHT_ANGLE,    s0},  /* unmatched right angle */
  /* 0D SIGN           */ {add_sign,       s2},  /* signed number? */
  /* 0E DECIMAL_POINT  */ {add_1st_decpt,  s4},  /* real number? */
  /* 0F ANY            */ {NAME,           s0},  /* executable name */
  /* 10 BREAK          */ {BREAK_SIGNAL,   s0},  /* break signalled */
  /* 11 EOF            */ {NO_TOKEN,       s0},  /* no token found */
 
  /* s1:  Further classify a '/' */
  /* 12 SLASH          */ {IMMED_NAME,     s0},  /* immediate name */
  /* 13 ANY            */ {LITERAL_NAME,   s0},  /* literal name */
  /* 14 BREAK          */ {BREAK_SIGNAL,   s0},  /* break signalled */
  /* 15 EOF            */ {OOPS_NAME,      s0},  /* isolated sign */
 
  /* s2:  sign */
  /* 16 WHITE_SPACE    */ {OOPS_NAME,      s0},  /* isolated sign */
  /* 17 SPECIAL        */ {OOPS_NAME,      s0},  /* isolated sign */
  /* 18 DIGIT          */ {add_digits,     s3},  /* number? */
  /* 19 DECIMAL_POINT  */ {add_decpt,      s4},  /* real number? */
  /* 1A ANY            */ {NAME,           s0},  /* executable name */
  /* 1B BREAK          */ {BREAK_SIGNAL,   s0},  /* break signalled */
  /* 1C EOF            */ {OOPS_NAME,      s0},  /* isolated sign */
 
  /* s3:  sign? digit+ */
  /* 1D WHITE_SPACE    */ {INTEGER,        s0},  /* n-digit integer */
  /* 1E SPECIAL        */ {INTEGER,        s0},  /* n-digit integer */
  /* 1F DECIMAL_POINT  */ {add_char,       s5},  /* real number? */
  /* 20 POUND          */ {add_radix,      s10}, /* radix number? */
  /* 21 eE             */ {add_char,       s7},  /* real with exponent? */
  /* 22 ANY            */ {AAH_NAME,       s0},  /* executable name */
  /* 23 BREAK          */ {BREAK_SIGNAL,   s0},  /* break signalled */
  /* 24 EOF            */ {INTEGER,        s0},  /* n-digit integer */
 
  /* s4:  sign? . */
  /* 25 WHITE_SPACE    */ {OOPS_NAME,      s0},  /* isolated +. or -. */
  /* 26 SPECIAL        */ {OOPS_NAME,      s0},  /* isolated +. or -. */
  /* 27 DIGIT          */ {add_fraction,   s6},  /* number? */
  /* 28 ANY            */ {NAME,           s0},  /* executable name */
  /* 29 BREAK          */ {BREAK_SIGNAL,   s0},  /* break signalled */
  /* 2A EOF            */ {OOPS_NAME,      s0},  /* isolated +. or -. */
 
  /* s5:  sign? digit+ . */
  /* 2B WHITE_SPACE    */ {REAL,           s0},  /* real with fraction */
  /* 2C SPECIAL        */ {REAL,           s0},  /* real with fraction */
  /* 2D eE             */ {add_char,       s7},  /* real with exponent? */
  /* 2E DIGIT          */ {add_fraction,   s6},  /* number? */
  /* 2F ANY            */ {AAH_NAME,       s0},  /* executable name */
  /* 30 BREAK          */ {BREAK_SIGNAL,   s0},  /* break signalled */
  /* 31 EOF            */ {REAL,           s0},  /* real with fraction */
 
  /* s6:  sign? (digit+ . digit+) | (. digit+) */
  /* 32 WHITE_SPACE    */ {REAL,           s0},  /* real with fraction */
  /* 33 SPECIAL        */ {REAL,           s0},  /* real with fraction */
  /* 34 eE             */ {add_char,       s7},  /* real with exponent? */
  /* 35 ANY            */ {AAH_NAME,       s0},  /* executable name */
  /* 36 BREAK          */ {BREAK_SIGNAL,   s0},  /* break signalled */
  /* 37 EOF            */ {REAL,           s0},  /* real with fraction */
 
  /* s7:  sign? ((digit+ (. digit*)?) | (. digit+)) Ee */
  /* 38 WHITE_SPACE    */ {OOPS_NAME,      s0},  /* invalid real number */
  /* 39 SPECIAL        */ {OOPS_NAME,      s0},  /* invalid real number */
  /* 3A SIGN           */ {add_e_sign,     s8},  /* real w signed exponent? */
  /* 3B DIGIT          */ {add_exponent,   s9},  /* real w exponent ? */
  /* 3C ANY            */ {AAH_NAME,       s0},  /* executable name */
  /* 3D BREAK          */ {BREAK_SIGNAL,   s0},  /* break signalled */
  /* 3E EOF            */ {OOPS_NAME,      s0},  /* invalid real number */
 
  /* s8:  sign? (digit+ (. digit*)? | (digit* . digit+) Ee sign */
  /* 3F WHITE_SPACE    */ {OOPS_NAME,      s0},  /* invalid real number */
  /* 40 SPECIAL        */ {OOPS_NAME,      s0},  /* invalid real number */
  /* 41 DIGIT          */ {add_exponent,   s9},  /* real w exponent? */
  /* 42 ANY            */ {AAH_NAME,       s0},  /* executable name */
  /* 43 BREAK          */ {BREAK_SIGNAL,   s0},  /* break signalled */
  /* 44 EOF            */ {OOPS_NAME,      s0},  /* invalid real number */
 
  /* s9:  sign? (digit+ (. digit*)? | (digit* . digit+) Ee sign? digit+ */
  /* 45 WHITE_SPACE    */ {REAL,           s0},  /* real w exponent */
  /* 46 SPECIAL        */ {REAL,           s0},  /* real w exponent */
  /* 47 ANY            */ {AAH_NAME,       s0},  /* executable name */
  /* 48 BREAK          */ {BREAK_SIGNAL,   s0},  /* break signalled */
  /* 49 EOF            */ {REAL,           s0},  /* real w exponent */
 
  /* s10: digit+ # */
  /* 4A WHITE_SPACE    */ {OOPS_NAME,      s0},  /* invalid radix number */
  /* 4B SPECIAL        */ {OOPS_NAME,      s0},  /* invalid radix number */
  /* 4C R_DIGIT        */ {add_r_digits,   s11}, /* radix number? */
  /* 4D ANY            */ {AAH_NAME,       s0},  /* executable name */
  /* 4E BREAK          */ {BREAK_SIGNAL,   s0},  /* break signalled */
  /* 4F EOF            */ {OOPS_NAME,      s0},  /* invalid radix number */
 
  /* s11: digit+ # r_digit+ */
  /* 50 WHITE_SPACE    */ {RADIX_NUMBER,   s0},  /* radix number */
  /* 51 SPECIAL        */ {RADIX_NUMBER,   s0},  /* radix number */
  /* 52 ANY            */ {AAH_NAME,       s0},  /* executable name */
  /* 53 BREAK          */ {BREAK_SIGNAL,   s0},  /* break signalled */
  /* 54 EOF            */ {RADIX_NUMBER,   s0}   /* radix number */
};
 
/*
 * Character Classification Tables --
 *
 *   The entries in the Character Classification Tables
 *   map character codes to character classes.  The
 *   tables contains one entry per code.  The bits in
 *   each entry indicate which classes the character
 *   code belongs to.
 *
 *   The macros 'isInCLASS(ch)' generate code to test
 *   whether 'ch' is a character in 'CLASS'.
 *
 */
/* Membership macros for classes defined in table 1 ... */
#define isRADIX_DIGIT(c)    ((isInP1[c] & 0x80) != 0)
#define isHEX_DIGIT(c)      ((isInP1[c] & 0x40) != 0)
#define isDECIMAL_DIGIT(c)  ((isInP1[c] & 0x10) != 0)
#define isOCTAL_DIGIT(c)    ((isInP1[c] & 0x20) != 0)
 
/* Membership macros for classes defined in table 2 ... */
#define isWHITE_SPACE(c)    ((isInP2[c] & 0x80) != 0)
#define isCOMMENT(c)        ((isInP2[c] & 0x40) != 0)
#define isNAME(c)           ((isInP2[c] & 0x20) != 0)
#define isSTRING_SPECIAL(c) ((isInP2[c] & 0x10) != 0)
#define isNUMBER_ENDER(c)   ((isInP2[c] & 0x08) != 0)
 
#define isInP1 (isInT1+2)
static unsigned char isInT1[258] = { 0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xD0,0xD0,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
 0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x00,0x00,
 0x00,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
 0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
 
#define isInP2 (isInT2+2)
static unsigned char isInT2[258] = { 0x18,0x18,
 0xC8,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0xC8,0x88,0x60,0x60,0x98,0x60,0x60,
 0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
 0xC8,0x60,0x60,0x60,0x60,0x48,0x60,0x60,0x58,0x58,0x60,0x60,0x60,0x60,0x60,0x48,
 0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x48,0x60,0x48,0x60,
 0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
 0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x48,0x70,0x48,0x60,0x60,
 0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
 0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x48,0x60,0x48,0x60,0x60,
 0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
 0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
 0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
 0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
 0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
 0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
 0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,
 0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60
};
 
#endif
