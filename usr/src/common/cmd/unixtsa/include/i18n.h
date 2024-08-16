/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)unixtsa:common/cmd/unixtsa/include/i18n.h	1.1"
/***
 *
 *  name	i18n.h - codeset conversion routines for unixtsa
 *		@(#)unixtsa:common/cmd/unixtsa/include/i18n.h	1.1	6/7/94
 *
 ***/

#ifndef _I18N_H
#define _I18N_H

#include	<stdio.h>
#include	<locale.h>
#include	<langinfo.h>
#include	<iconv.h>

#define	CONVERSION_SIZE		256 // maximum length of converted string

/* globals variables */
extern iconv_t	TsaToEngine, EngineToTsa ;

/* prototypes */
int	ConvertInitialize( char *TsaSet, char *EngineSet ) ;
char	*ConvertTsaToEngine( char *Source, size_t *NewSize ) ;
char	*ConvertEngineToTsa( char *Source, size_t *NewSize ) ;

#endif /* _I18N_H */
