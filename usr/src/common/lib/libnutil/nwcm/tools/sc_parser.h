/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/nwcm/tools/sc_parser.h	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: sc_parser.h,v 1.3 1994/09/22 18:49:22 vtag Exp $"
/*
 * Copyright 1992 Unpublished Work of Novell, Inc.
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

#ifndef __SC_PARSER_H__
#define __SC_PARSER_H__

/* sc_parser.h
 *
 *	Definitions that must be held in common between the various
 *	parts of the schema compiler.
 */

#define	SCHEMA_COMPILER
#include "schemadef.h"

enum VM { NO_VALIDATION, MIN_MAX_VALIDATION, FUNCTION_VALIDATION };

extern int	lineno;
extern char *	filename;

extern void	Ident(char *);
extern void	IntegerParamDef(char *, char *, char *, char *, enum df_e, unsigned long, enum VM, ...);
extern void	BooleanParamDef(char *, char *, char *, char *, int, char *);
extern void	StringParamDef(char *, char *, char *, char *, enum df_e, char *, char *);

#endif /* __SC_PARSER_H__ */
