/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:schemadef.h	1.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

# /* ident	"$Header: /SRCS/esmp/usr/src/nw/head/schemadef.h,v 1.3 1994/08/30 15:36:48 mark Exp $" */
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

#ifndef __SCHEMADEF_H__
#define __SCHEMADEF_H__

#ifdef NWCM_FRIEND

/* schemadef.h
 *
 *	Defines the data types that describe the data produced by
 *	the schema compiler/code-generator.  This is private to
 *	the configuration manager.
 */

#include <nwconfig.h>

#ifdef SCHEMA_COMPILER
typedef	char	*ivf_t;			/* name of integer validation func */
typedef	char	*svf_t;			/* name of string validation func */
typedef	char	*bvf_t;			/* name of string validation func */
typedef int	cv_t;			/* index of configuration value */
typedef char	*pinfo_t;	/* name of parameter information constants */
#else
#ifdef __STDC__
typedef	int	(*ivf_t)(unsigned long);/* pointer to integer validation func */
typedef	int	(*svf_t)(char *);	/* pointer to string validation func */
typedef	int	(*bvf_t)(int);		/* pointer to boolean action func */
#else /* __STDC__ */
typedef	int	(*ivf_t)();		/* pointer to integer validation func */
typedef	int	(*svf_t)();		/* pointer to string validation func */
typedef	int	(*bvf_t)();		/* pointer to boolean action func */
#endif /* __STDC__ */
typedef void	*cv_t;			/* pointer to configuration value */
typedef int		pinfo_t;	/* parameter information */
#endif

/* validation data for integer parameters */
struct ipvd_s {
#ifdef SCHEMA_COMPILER
	int		index;		/* self reference for offset */
#endif
	ivf_t		func;
	unsigned long	min, max;
};

/* display format */
enum df_e {
	df_normal = 0,
	df_decimal,
	df_octal,
	df_hexadecimal,
	df_uppercase
};

/* configuration parameter data */
struct cp_s {
	char		*name;
	pinfo_t		folder;
	int			rel_order;		/* order relative to schema def */
	pinfo_t		description;	/* message number */
	pinfo_t		helpString;		/* message number */
	enum NWCP	type;
	union		{
#ifndef SCHEMA_COMPILER
		void		*opaque;	/* generic for agr. init. */
#endif
		svf_t		func;
		bvf_t		action;
		struct ipvd_s	*data;
	}		validation;
	enum df_e	format;
	cv_t		def_val;
#ifndef SCHEMA_COMPILER
	cv_t		cur_val;
#endif
};

#ifndef SCHEMA_COMPILER
/* Defines for binary file building */
extern char			intValFuncs[][NWCM_MAX_STRING_SIZE];
extern char			stringValFuncs[][NWCM_MAX_STRING_SIZE];
extern char			boolValFuncs[][NWCM_MAX_STRING_SIZE];
extern struct cp_s	ConfigParameters[];
extern unsigned long IntParameterDefaults[];

/* Defines for binary file reading */
extern unsigned long	*IntegerParameterValues;
extern unsigned long	*IntegerParameterDefaults;
extern int		IntegerParameterCount;
extern int		*BooleanParameterValues;
extern int		*BooleanParameterDefaults;
extern int		BooleanParameterCount;
extern char		*StringParameterValues;
extern char		*StringParameterDefaults;
extern int		StringParameterCount;
extern struct cp_s	*ConfigurationParameters;
extern int		ConfigurationParameterCount;
#endif

#endif /* NWCM_FRIEND */

#endif /* __SCHEMADEF_H__ */
