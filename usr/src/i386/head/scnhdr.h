/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs-head:i386/head/scnhdr.h	1.14"

#ifndef _SCNHDR_H
#define _SCNHDR_H

struct scnhdr {
	char		s_name[8];	/* section name */
	long		s_paddr;	/* physical address, aliased s_nlib */
	long		s_vaddr;	/* virtual address */
	long		s_size;		/* section size */
	long		s_scnptr;	/* file ptr to raw data for section */
	long		s_relptr;	/* file ptr to relocation */
	long		s_lnnoptr;	/* file ptr to line numbers */
#ifdef m88k
	long		s_nreloc;	/* number of relocation entries */
	union {
		long	su_nlnno;	/* number of line number entries */
		long	su_vendor;	/* BCS effluvia */
	} s_u;
#define s_nlnno		s_u.su_nlnno
#define s_vendor	s_u.su_vendor
#else
	unsigned short	s_nreloc;	/* number of relocation entries */
	unsigned short	s_nlnno;	/* number of line number entries */
#endif
	long		s_flags;	/* flags */
};

/* the number of shared libraries in a .lib section in an absolute output file
 * is put in the s_paddr field of the .lib section header, the following define
 * allows it to be referenced as s_nlib
 */

#define s_nlib	s_paddr
#define	SCNHDR	struct scnhdr
#define	SCNHSZ	sizeof(SCNHDR)

/*
 * Define constants for names of "special" sections
 */

#define _TEXT ".text"
#define _DATA ".data"
#define _DATA1 "data1"
#define _BSS  ".bss"
#define _TV  ".tv"
#define _INIT ".init"
#define _RODATA ".rodata"
#define _FINI ".fini"
#define _SHADOW ".shadow"
#ifdef m88k
#define _EXPR ".expr"
#define _COMMENT ".comment"
#endif

/*
 * The low 2 bytes of s_flags is used as a section "type"
 */

#define STYP_REG	0x00		/* "regular" section:
						allocated, relocated, loaded */
#define STYP_DSECT	0x01		/* "dummy" section:
						not allocated, relocated,
						not loaded */
#define STYP_NOLOAD	0x02		/* "noload" section:
						allocated, relocated,
						 not loaded */
#define STYP_GROUP	0x04		/* "grouped" section:
						formed of input sections */
#define STYP_PAD	0x08		/* "padding" section:
						not allocated, not relocated,
						 loaded */
#define STYP_COPY	0x10		/* "copy" section:
						for decision function used
						by field update;  not
						allocated, not relocated,
						loaded;  reloc & lineno
						entries processed normally */
#define STYP_INFO	0x200		/* comment section : not allocated
						not relocated, not loaded */
#define STYP_LIB	0x800		/* for .lib section : same as INFO */
#define STYP_OVER	0x400		/* overlay section : relocated
						not allocated or loaded */
#ifdef m88k
/* ZZZ	the below definition (SHADOW) is a bit fishy 	*/
#define	STYP_SHADOW	0x2000
#else
#define	STYP_SHADOW	0x1000
#endif
					/* shadow section:
						not a cohesive section -
						contains C++ initialized data.
						Data is broken up and 
						relocated in final ld. */
#define	STYP_TEXT	0x20		/* section contains text only */
#define STYP_DATA	0x40		/* section contains data only */
#define STYP_BSS	0x80		/* section contains bss only */
#ifdef m88k
#define STYP_VENDOR	0x1000		/* new BCS vendor specific section */
#endif
#ifdef i860
#define STYP_ABS	0x100		/* section contains abs only */
#endif

/*
 *  In a minimal file or an update file, a new function
 *  (as compared with a replaced function) is indicated by S_NEWFCN
 */

#define S_NEWFCN  0x100

/*
 * In 3b Update Files (output of ogen), sections which appear in SHARED
 * segments of the Pfile will have the S_SHRSEG flag set by ogen, to inform
 * dufr that updating 1 copy of the proc. will update all process invocations.
 */

#define S_SHRSEG	0x20

#endif 	/* _SCNHDR_H */
