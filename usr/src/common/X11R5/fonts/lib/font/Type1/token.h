/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fonts:lib/font/Type1/token.h	1.1"

/* $XConsortium: token.h,v 1.2 91/10/10 11:19:58 rws Exp $ */
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
#ifndef TOKEN_H
#define TOKEN_H
 
/* Special characters */
#define CONTROL_C           (3)
 
/* Token type codes */
#define TOKEN_INVALID       (-3)
#define TOKEN_BREAK         (-2)
#define TOKEN_EOF           (-1)
#define TOKEN_NONE          (0)
#define TOKEN_LEFT_PAREN    (1)
#define TOKEN_RIGHT_PAREN   (2)
#define TOKEN_LEFT_ANGLE    (3)
#define TOKEN_RIGHT_ANGLE   (4)
#define TOKEN_LEFT_BRACE    (5)
#define TOKEN_RIGHT_BRACE   (6)
#define TOKEN_LEFT_BRACKET  (7)
#define TOKEN_RIGHT_BRACKET (8)
#define TOKEN_NAME          (9)
#define TOKEN_LITERAL_NAME  (10)
#define TOKEN_INTEGER       (11)
#define TOKEN_REAL          (12)
#define TOKEN_RADIX_NUMBER  (13)
#define TOKEN_HEX_STRING    (14)
#define TOKEN_STRING        (15)
#define TOKEN_IMMED_NAME    (16)
 
/* Token routines */
extern void scan_token();
 
/*
 * -------------------------------------------------------------------------
 * Globals shared  -- (everyone else KEEP YOUR MITTS OFF THEM!)
 * -------------------------------------------------------------------------
 */
 
/* These variables are set by the caller */
extern char     *tokenStartP; /* Pointer to token buffer in VM */
extern char     *tokenMaxP;   /* Pointer to end of VM we may use + 1 */
 
/* These variables are set by P_TOKEN */
extern int      tokenLength;  /* Characters in token */
extern boolean  tokenTooLong; /* Token too long for space available */
extern int      tokenType;    /* Type of token identified */
extern psvalue  tokenValue;   /* Token value */
 
#endif /* TOKEN_H */
