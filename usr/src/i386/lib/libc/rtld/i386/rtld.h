/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rtld:i386/rtld.h	1.13"

/* common header for run-time linker */

/* structure used to determine whether a particular shared library
 * is part of a given group, opened in a single call to rtld;
 * objects may reference symbols defined in any of the other objects 
 * belonging to the same groups to which they belong, or in any
 * object with the refpermit bit set
 */
struct rt_set {
	unsigned long	members;
	struct rt_set	*next;
};

/* struct of fd descriptors and path names used in
 * opening objects;
 */
struct namelist {
	char	*n_name;
	int	n_fd;
	dev_t	n_dev;
	ino_t	n_ino;
};

/* linked list of rt_private maps; used to keep track of
 * dependencies
 */
struct maplist {
	struct rt_private_map *l_map;
	struct maplist *l_next;
};

typedef struct maplist	mlist;

/* run-time linker private data maintained for each shared object -
 * connected to link_map structure for that object
 */

struct rt_private_map {
	struct link_map r_public;	/* public data */
	VOID *r_symtab;			/* symbol table */
	unsigned long *r_hash;		/* hash table */
	char *r_strtab;			/* string table */
	VOID *r_reloc;			/* relocation table */
	unsigned long *r_pltgot;	/* addresses for procedure linkage table */
	VOID *r_jmprel;			/* plt relocations */
	unsigned long r_pltrelsize;	/* size of PLT relocation entries */
	void (*r_init)();		/* address of _init */
	void (*r_fini)();		/* address of _fini */
	unsigned long r_relsz;		/* size of relocs */
	unsigned long r_msize;		/* total memory mapped */
	unsigned long r_entry;		/* entry point for file */
	VOID  *r_phdr;			/* program header of object */
	unsigned short r_phnum;		/* number of segments */
	unsigned short r_phentsize;	/* size of phdr entry */
	unsigned short r_relent;	/* size of base reloc entry */
	unsigned short r_syment;	/* size of symtab entry */
	unsigned short r_count;		/* reference count */
	unsigned short r_flags;		/* set of flags */
	dev_t	r_dev;			/* device for file */
	ino_t	r_ino;			/* inode for file */
	mlist	*r_needed;		/* needed list for this object */
	mlist *r_reflist;		/* list of references */
	struct rt_set r_grpset;		/* which groups does this 
					 * object belong to? */
};

/* definitions for use with flags field */
#define RT_SYMBOLIC	0x1
#define RT_TEXTREL	0x2
#define RT_NODELETE	0x4
#define RT_REFERENCED	0x8
#define RT_BIND_NOW	0x10
#define RT_INIT_CALLED	0x20
#define RT_FINI_CALLED	0x40
#define RT_NEEDED_SEEN  0x80

/* macros for getting to link_map data */
#define ADDR(X) ((X)->r_public.l_addr)
#define NAME(X) ((X)->r_public.l_name)
#define DYN(X) ((X)->r_public.l_ld)
#define NEXT(X) ((X)->r_public.l_next)
#define PREV(X) ((X)->r_public.l_prev)
#define TEXTSTART(X)	((X)->r_public.l_tstart)
#define TEXTSIZE(X)	((X)->r_public.l_tsize)

/* macros for getting to linker private data */
#define SET_FLAG(X, F) ((X)->r_flags |= (F))
#define CLEAR_FLAG(X, F) ((X)->r_flags &= ~(F))
#define TEST_FLAG(X, F) ((X)->r_flags & (F))
#define COUNT(X) ((X)->r_count)
#define SYMTAB(X) ((X)->r_symtab)
#define HASH(X) ((X)->r_hash)
#define STRTAB(X) ((X)->r_strtab)
#define PLTGOT(X) ((X)->r_pltgot)
#define JMPREL(X) ((X)->r_jmprel)
#define PLTRELSZ(X) ((X)->r_pltrelsize)
#define INIT(X) ((X)->r_init)
#define FINI(X) ((X)->r_fini)
#define RELSZ(X) ((X)->r_relsz)
#define REL(X) ((X)->r_reloc)
#define RELENT(X) ((X)->r_relent)
#define SYMENT(X) ((X)->r_syment)
#define MSIZE(X) ((X)->r_msize)
#define ENTRY(X) ((X)->r_entry)
#define RPATH(X) ((X)->r_runpath)
#define PHDR(X) ((X)->r_phdr)
#define PHNUM(X) ((X)->r_phnum)
#define PHSZ(X) ((X)->r_phentsize)
#define REFLIST(X) ((X)->r_reflist)
#define GRPSET(X)  (&((X)->r_grpset))
#define NEEDED(X)  ((X)->r_needed)
#define DEV(X)  ((X)->r_dev)
#define INO(X)  ((X)->r_ino)
#define NEXT_FINI(X)  ((X)->r_next_fini)


/* data structure used to keep track of special R1_COPY
 * relocations
 */

struct rel_copy	{
	VOID *r_to;		/* copy to address */
	VOID *r_from;		/* copy from address */
	unsigned long r_size;		/* copy size bytes */
	struct rel_copy *r_next;	/* next on list */
};

/* Elf32_Dyn tags used in dlopen/rtld interface */
#define	DT_FPATH	(-1)	/* pathname of file to be opened */
#define	DT_MODE		(-2)	/* function binding mode */
#define	DT_MAP		(-3)	/* pointer to link_map */
#define	DT_GROUP	(-4)	/* dlopen group id */

#define DT_MAXNEGTAGS	4	/* number of negative tags */

/* debugger information version*/
#define LD_DEBUG_VERSION 1

#ifdef DEBUG
#define MAXLEVEL	7	/* maximum debugging level */
#define LIST		1	/* dubgging levels - or'able flags */
#define DRELOC		2
#define MAP		4
#endif

/* flags for lookup routine */
#define	LOOKUP_NORM	0	/* normal action for a.out undefines */
#define	LOOKUP_SPEC	1	/* special action for a.out undefines */

