/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/ast/symbols_ast.c	1.1"
#ident	"$Header: $"

/*
 * Generate symbols for use by assembly language files in kernel.
 *
 * This file is compiled using "-S"; "symbols.awk" walks over the assembly
 * file and extracts the symbols.
 */

#include <ebi.h>
#include <ast.h>

#define	offsetof(x, y)	((int)&((x *)0)->y)
#define	OFFSET(s, st, m) \
	size_t __SYMBOL___A_##s = offsetof(st, m)

#define	DEFINE(s, e) \
	size_t __SYMBOL___A_##s = (size_t)(e)

/*
 * Offsets into the EBI II jump table
 */

OFFSET(MASKABLEINTEOI, EBI_II, MaskableIntEOI);
OFFSET(REGSETLOCALINTMASK, EBI_II, RegSetLocalIntMask);

/*
 * defines for SPI and IPI interrupt vector.
 */
DEFINE(SPI, SPI);
DEFINE(IPI, IPI);

