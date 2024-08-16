/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_AUTOCONF_CONFMGR_CONFMGR_H
#define _IO_AUTOCONF_CONFMGR_CONFMGR_H

#ident	"@(#)kern:io/autoconf/confmgr/confmgr.h	1.13"
#ident	"$Header: $"

#if defined ( __cplusplus )
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <io/autoconf/resmgr/resmgr.h>
#include <util/types.h>

#else

#include <sys/resmgr.h>
#include <sys/types.h>

#endif /* _KERNEL_HEADERS */

/*
** These are the platform-independent pre-defined parameters and
** if applicable, some symbolic values.
*/

#define CM_TYPE		"TYPE"		/* tag different types of records */
#define CM_MODNAME	"MODNAME"	/* Module Name */
#define CM_ENTRYTYPE	"ENTRYTYPE"	/* Type of RM database entry */

/* Values for CM_TYPE */

#define CM_TYPE_BRDINST		1	/* Board Instance */

/* Symbolic Entry types for CM_ENTRYTYPE */

#define CM_ENTRY_DEFAULT	0x01	/* Added by idresadd & idconfupdate -s */
#define CM_ENTRY_DRVOWNED	0x02	/* PCU data interpreted and maintained
					** by driver
					*/

/*
** Maximum length for MODNAME, including '\0'.  This #define was pulled
** from the idtools header inst.h.  It MUST match that define.
*/

#define NAMESZ		15

#ifdef _KERNEL

#define CM_VERSION	1	/* The current version of confmgr */

/*
** cm_args is the input/output argument for those confmgr routines
** that interface with the resmgr database.
*/

typedef struct cm_args
{
	rm_key_t	cm_key;
	char		*cm_param;
	void		*cm_val;
	size_t		cm_vallen;
	uint_t		cm_n;
} cm_args_t;

#ifdef __STDC__

extern int	cm_getversion( void );
extern int	cm_getnbrd( char * );
extern rm_key_t	cm_getbrdkey( char *, int );
extern int	cm_delval( cm_args_t * );
extern int	cm_addval( cm_args_t * );
extern int	cm_getval( cm_args_t * );
extern int	cm_intr_attach( rm_key_t, void (*)(), int *, void ** );
extern void	cm_intr_detach( void * );
extern void	cm_intr_attach_all( char *, void (*)(), int *, void ** );
extern void	cm_intr_detach_all( void * );
extern int	cm_read_devconfig( rm_key_t, off_t, void *, size_t );
extern int	cm_write_devconfig( rm_key_t, off_t, void *, size_t );
extern size_t	cm_devconfig_size( rm_key_t );

#else

extern int	cm_getversion();
extern int	cm_getnbrd();
extern rm_key_t	cm_getbrdkey();
extern int	cm_delval();
extern int	cm_addval();
extern int	cm_getval();
extern int	cm_intr_attach();
extern void	cm_intr_detach();
extern void	cm_intr_attach_all();
extern void	cm_intr_detach_all();
extern int	cm_read_devconfig();
extern int	cm_write_devconfig();
extern size_t	cm_devconfig_size();

#endif /* __STDC__ */

#endif /* _KERNEL */

#if defined ( __cplusplus )
	}
#endif

#endif /* _IO_AUTOCONF_CONFMGR_CONFMGR_H */
