/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)make:files.c	1.21.1.3"

#include "defs"
#include <ar.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <ccstypes.h>
#include <ctype.h>
#include <pfmt.h>
#include <locale.h>

#define STREQN		!strncmp
#define MAXOPFIL	10	/* number of files that can be open */

extern	PATTERN	firstpat;	/* main.c */


/* UNIX VERSION DEPENDENT PROCEDURES */

/*
 * For 6.0, create a make which can understand all three archive
 * formats.  This is kind of tricky, and <ar.h> isn't any help.
 */
char	archmem[64];	/* archive file member name to search for */

int	ar_type;	/* to distiguish which archive format we have */
#define ARold	1
#define AR5	2
#define ARport	3

long	first_ar_mem,	/* where first archive member header is at */
	sym_begin,	/* where the symbol lookup starts */
	num_symbols,	/* the number of symbols available */
	sym_size;	/* length of symbol directory file */

/*
 * Defines for all the different archive formats.  See next comment
 * block for justification for not using <ar.h>'s versions.
 */
#define ARoldMAG	0177545	/* old format (short) magic number */

#define AR5MAG		"<ar>"	/* 5.0 format magic string */
#define SAR5MAG		4	/* 5.0 format magic string length */

#define ARportMAG	"!<arch>\n"	/* Port. (6.0) magic string */
#define SARportMAG	8		/* Port. (6.0) magic string length */
#define ARFportMAG	"`\n"		/* Port. (6.0) end of header string */

/*
* These are the archive file headers for the three formats.  Note
* that it really doesn't matter if these structures are defined
* here.  They are correct as of the respective archive format
* releases.  If the archive format is changed, then since backwards 
* compatability is the desired behavior, a new structure is added
* to the list.
*/
struct {	/*  old archive format */
	char	ar_name[14];	/* '\0' terminated */
	long	ar_date;	/* native machine bit representation */
	char	ar_uid;		/* 	"	*/
	char	ar_gid;		/* 	"	*/
	int	ar_mode;	/* 	"	*/
	long	ar_size;	/* 	"	*/
} ar_old;

struct {	/* old a.out header */
	short	a_magic;
	unsigned	a_text;
	unsigned	a_data;
	unsigned	a_bss;
	unsigned	a_syms;		/* length of symbol table */
	unsigned	a_entry;
	char	a_unused;
	char	a_hitext;
	char	a_flag;
	char	a_stamp;
} arobj_old;

struct {	/* old a.out symbol table entry */
	char	n_name[8];	/* null-terminated name */
	int	n_type;
	unsigned	n_value;
} ars_old;

struct {	/* UNIX 5.0 archive header format: 3b */
	char	ar_magic[SAR5MAG];	/* AR5MAG */
	char	ar_name[16];		/* ' ' terminated */
	char	ar_date[4];		/* sgetl() accessed */
	char	ar_syms[4];		/* sgetl() accessed */
} arh_5;

struct {	/* UNIX 5.0 archive symbol format: 3b */
	char	sym_name[8];	/* ' ' terminated */
	char	sym_ptr[4];	/* sgetl() accessed */
} ars_5;

struct {	/* UNIX 5.0 archive member format: 3b */
	char	arf_name[16];	/* ' ' terminated */
	char	arf_date[4];	/* sgetl() accessed */
	char	arf_uid[4];	/*	"	*/
	char	arf_gid[4];	/*	"	*/
	char	arf_mode[4];	/*	"	*/
	char	arf_size[4];	/*	"	*/
} arf_5;

struct { 	/* Portable (6.0) archive format: 3b */
	char	ar_name[16];	/* '/' terminated */
	char	ar_date[12];	/* left-adjusted; decimal ascii; blank filled */
	char	ar_uid[6];	/*	"	*/
	char	ar_gid[6];	/*	"	*/
	char	ar_mode[8];	/* left-adjusted; octal ascii; blank filled */
	char	ar_size[10];	/* left-adjusted; decimal ascii; blank filled */
	char	ar_fmag[2];	/* special end-of-header string (ARFportMAG) */
} ar_port;



FILE	*arfd;

int	nopen = 0;

/*
**	Declare local functions and make LINT happy.
*/

static DEPBLOCK	dodir();
static int	umatch();
static time_t	afilescan();
static time_t	entryscan();
static time_t	oldentrys();
static int	openarch();
static DIR *	getfid();
static time_t	lookarch();

static	char *name_table = NULL;
static	size_t	length = 0;


DEPBLOCK
srchdir(pat, mkchain, nextdbl)
register CHARSTAR pat;	/* pattern to be matched in directory */
int	mkchain;	/* nonzero if results to be remembered */
DEPBLOCK nextdbl;       /* final value for chain */
{
	extern char * cur_wd;
	register PATTERN patp;
	DIR 	*dirf;
	CHARSTAR dirname, dirpref, filepat, dname(), sname();
	char	temp[MAXPATHLEN], temp2[MAXPATHLEN];
	char	pattemp[MAXPATHLEN], dirtemp[MAXPATHLEN];

	if ( !mkchain )
		for (patp = firstpat; patp; patp = patp->nextpattern)
			if (STREQ(pat, patp->patval))
				return(0);

	patp = ALLOC(pattern);
	patp->nextpattern = firstpat;
	firstpat = patp;
	patp->patval = copys(pat);

	(void)copstr(pattemp, pat);
	(void)copstr(dirtemp, pat);
	(void)dname(dirtemp);
	if (STREQ(dirtemp, ".")) {
		dirpref = "";
		dirname = "";
	} else {
		dirpref = concat(dirtemp, "/", temp);
		dirname = concat("/", dirtemp, temp2);
	}

	filepat = sname(pattemp);
	if (*pat == '/') {
		if ( !(dirf = getfid(dirname)) )
			return(nextdbl);
		else if (dirf != (DIR *) -1)
			nextdbl = dodir(dirf, filepat, dirpref, nextdbl, mkchain);
	} else{
		(void)concat(cur_wd, dirname, dirtemp);
#ifdef MKDEBUG
		if (IS_ON(DBUG))
			printf("looking in [%s]\n", dirtemp);
#endif
		if ( dirf = getfid(dirtemp))
			if (dirf != (DIR *) - 1)
				nextdbl = dodir(dirf, filepat, dirpref, nextdbl, mkchain);
	}
	return(nextdbl);
}


FSTATIC char file_name[MAXNAMLEN];


static DEPBLOCK
dodir(dirf, filepat, dirpref, nextdbl, mkchain)
DIR	*dirf;
register CHARSTAR filepat;
CHARSTAR dirpref;
DEPBLOCK nextdbl;
int	mkchain;
{
	register CHARSTAR p1, p2;
	DEPBLOCK thisdbl;
	register struct dirent * entry;
	char	fullname[MAXPATHLEN];
	int	amatch();
	NAMEBLOCK q;

	while((entry = readdir(dirf)) != NULL) {
		p1 = entry->d_name;
		p2 = file_name;

		while ( (*p2++ = *p1++) != CNULL ) ;

		if ( amatch(file_name, filepat) ) {
			(void)concat(dirpref, file_name, fullname);
			if ( !(q = SRCHNAME(fullname)) )
				q = makename(copys(fullname));

			if (mkchain) {
				thisdbl = ALLOC(depblock);
				thisdbl->nextdep = nextdbl;
				thisdbl->depname = q;
				nextdbl = thisdbl;
			}
		}
	}

	return (nextdbl);
}


amatch(s, p)		/* stolen from glob through find */
register CHARSTAR p;
CHARSTAR s;
{
	register int	cc, scc = *s, k;
	int	c, lc = LRGINT;

	switch (c = *p) {

	case LSQUAR:
		k = 0;
		while (cc = *++p) {
			switch (cc) {

			case RSQUAR:
				if (k)
					return(amatch(++s, ++p));
				return(0);

			case MINUS:
				k |= (((lc <= scc) & (scc <= (cc = p[1]))));
			}
			if (scc == (lc = cc))
				k++;
		}
		return(0);

	case QUESTN:
caseq:		if (scc)
			return(amatch(++s, ++p));
		return(0);

	case STAR:
		return(umatch(s, ++p));

	case 0:
		return(!scc);
	}

	if (c == scc)
		goto caseq;

	return(0);
}


static int
umatch(s, p)
register CHARSTAR s, p;
{
	if ( !(*p) )
		return(1);
	while (*s)
		if (amatch(s++, p))
			return(1);
	return(0);
}


time_t
la(archive, member, flag)
char	*archive, *member;
int	flag;
{
	time_t	date;
	int	aropen = openarch(archive);

	if ((aropen < 0) && arfd)
		(void)fclose( arfd );

	if (aropen == -1)	/* can't open archive */
		return(0L);

/*
 *	Openarch returns -1 if the archive is not able to be opened.
 *	Openarch has been modified to allow distinction between archive
 *	open problems (-1) and symbol table problems for 6.0 archives(-2).
 */
	if (aropen == -2)	/* NULL archive */
		return(1L);

	if (flag)
		date = entryscan(archive, member);
	else
		date = afilescan(archive, member);

	/* close the archive file */
	(void)fclose( arfd );

	return(date);
}


static time_t
afilescan(an, name)	/* return date for named archive member file */
char	*an, *name;
{
	long sgetl();
	long	ptr;
	size_t	len = strlen(name);

	if (fseek(arfd, first_ar_mem, 0))
seek_error:	fatal1(":282:seek error on archive %s", an);

	/*
	* Hunt for the named file in each different type of
	* archive format.
	*/
	switch (ar_type) {
	case ARold:
		for (; ; ) {
			if (fread((char *) &ar_old,
				   sizeof(ar_old), 1, arfd) != 1) {
				if (feof(arfd))
					return (1L);
				break;
			}
			if (STREQN(ar_old.ar_name, name, len))
				return (ar_old.ar_date);
			ptr = ar_old.ar_size;
			ptr += (ptr & 01);
			if (fseek(arfd, ptr, 1))
				goto seek_error;
		}
		break;
	case AR5:
		for (; ; ) {
			if (fread((char *) &arf_5,
				  sizeof(arf_5), 1, arfd) != 1) {
				if (feof(arfd))
					return (1L);
				break;
			}
			if (STREQN(arf_5.arf_name, name, len))
				return (sgetl(arf_5.arf_date));
			ptr = sgetl(arf_5.arf_size);
			ptr += (ptr & 01);
			if (fseek(arfd, ptr, 1))
				goto seek_error;
		}
		break;
	case ARport:
		for (; ; ) {
			if ((fread((char *) &ar_port,
				    sizeof(ar_port), 1, arfd) != 1) ||
			    !(STREQN(ar_port.ar_fmag, ARFportMAG,
				      sizeof(ar_port.ar_fmag)))) {
				if (feof(arfd))
					return (1L);
				break;
			}
			if (ar_port.ar_name[0] == '/') {
				if ( ar_port.ar_name[1] == '/') {
				/* a name starting with a // indicates that there
				 * is a table strings containing archive member
				 * names that are longer than 16 characters
				 */
				 	if(!name_table) {
						off_t whence;
			
						/* have to read in the string table
						 * for the first time 
						 */
						whence = ftell(arfd);
						sscanf(ar_port.ar_size, "%ld", &length);
						name_table = (char *)malloc(length);
						if(fread(name_table, length, 1, arfd) != 1) {
							if (feof(arfd))
								return (1L);
							break;
						}
						fseek(arfd, whence, SEEK_SET);
				         }
			       	}
			       	/* look for name in string table */
			       	else if (ar_port.ar_name[1] != '/') {
				   int offset;
	
				   sscanf(&(ar_port.ar_name[1]), "%d", &offset);
				   if (STREQN(&(name_table[offset]), name, len)) {
					long	date;
	
					if (sscanf(ar_port.ar_date, "%ld", 
					    &date) != 1)
						fatal1(":283:Bad date field for %.14s in %s",
					    		name, an);
						return (date);
				   	}
				}
		    	}
			if (STREQN(ar_port.ar_name, name, len) &&
			    (len == sizeof ar_port.ar_name ||
			     ar_port.ar_name[len] == '/' ||
			     ar_port.ar_name[len] == ' ' ||
			     ar_port.ar_name[len] == CNULL )) {
				long	date;

				if (sscanf(ar_port.ar_date, "%ld", &date) != 1)
					fatal1(":283:Bad date field for %.14s in %s",
					    name, an);

				return (date);
			}
			if (sscanf(ar_port.ar_size, "%ld", &ptr) != 1)
				fatal1(":284:Bad size field for %.14s in archive %s",
					    name, an);

			ptr += (ptr & 01);
			if (fseek(arfd, ptr, 1))
				goto seek_error;
		}
		break;
	}

	/* Only here if fread() [or STREQN()] failed and not at EOF */

	fatal1(":285:read error on archive %s", an);
}


static time_t
entryscan(an, name)	/* return date of member containing global var named */
char	*an, *name;
{
	long 	sgetl();

	/*
	* Hunt through each different archive format for the named
	* symbol.  Note that the old archive format does not support
	* this convention since there is no symbol directory to scan
	* through for all defined global variables. 
	*/
	if (ar_type == ARold)
		return (oldentrys(an, name));
	if (sym_begin == 0L || num_symbols == 0L)
no_such_sym:	fatal1(":286:cannot find symbol %s in archive %s",
			name, an);

	if (fseek(arfd, sym_begin, 0))
seek_error:	fatal1(":282:seek error on archive %s", an);

	if (ar_type == AR5) {
		register int	i;
		unsigned int	len = strlen(name);

		if (len > 8)
			len = 8;
		for (i = 0; i < num_symbols; i++) {
			if (fread((char *) &ars_5, sizeof(ars_5), 1, arfd) != 1)
read_error:			fatal1(":285:read error on archive %s",
					an);
			if (STREQN(ars_5.sym_name, name, len)) {
				if (fseek(arfd, sgetl(ars_5.sym_ptr), 0))
					goto seek_error;
				if (fread((char *)&arf_5,
					  sizeof(arf_5), 1, arfd) != 1)
					goto read_error;

				/* replace symbol name w/ member name */
				(void)strncpy(archmem, arf_5.arf_name,
					sizeof(arf_5.arf_name));

				return (sgetl(arf_5.arf_date));
			}
		}
	} else {	/* ar_type == ARport */
		register CHARSTAR offs,		/* offsets table */
			 	  syms,		/* string table */
				strend;		/* end of string table */
		void	free();
		int	strtablen;
		CHARSTAR strbeg;

		/*
		* Format of the symbol directory for this format is
		* as follows:	[sputl()d number_of_symbols]
		*		[sputl()d first_symbol_offset]
		*			...
		*		[sputl()d number_of_symbols'_offset]
		*		[null_terminated_string_table_of_symbols]
		*/
		if ( !(offs = (char *) malloc( (unsigned) (num_symbols * sizeof(long)))) )
			fatal1(":287:cannot alloc offsets table for archive %s",
				an);

		if (fread(offs, sizeof(long), (int) num_symbols, arfd) != num_symbols)
			goto read_error;

		strtablen = sym_size - ((num_symbols + 1L) * sizeof(long));
		if ( !(syms = (char *) malloc((unsigned) strtablen)) )
			fatal1(":288:cannot alloc string table for archive %s",
			    an);

		if (fread(syms, sizeof(char), strtablen, arfd) != strtablen)
			goto read_error;
		strbeg = syms;
		strend = &syms[strtablen];
		/* while less than end of string table */
		while (syms < strend) {
			if (STREQ(syms, name)) {
				long	date;
				register char *ap, *hp;

				if (fseek(arfd, sgetl(offs), 0))
					goto seek_error;
				if ((fread((char *) &ar_port,
					sizeof(ar_port), 1, arfd) != 1) ||
				    !(STREQN(ar_port.ar_fmag,
				    		ARFportMAG,
						sizeof(ar_port.ar_fmag))))
					goto read_error;

				if (sscanf(ar_port.ar_date, "%ld", &date) != 1)
					fatal1(":289:Bad date for %.14s, archive %s",
					    ar_port.ar_name, an);

				/* replace symbol name w/ member name */
				ap = archmem;
				hp = ar_port.ar_name;
				while (*hp && *hp != '/' &&
				       (ap < archmem + sizeof(archmem)))
					*ap++ = *hp++;

				free(strbeg);
				return (date);
			}
			syms += strlen(syms) + 1;
			offs += sizeof(long);
		}
		free(strbeg);
	}
	goto no_such_sym;
}


static time_t
oldentrys(an, name)
char	*an, *name;
{
	register int	i;
	long 	ftell(), skip, last, len = strlen(name);

	fatal(":290:Cannot do global variable search old object file.");
	if (len > 8L)
		len = 8L;
	/*
	* Look through archive, an object file entry at a time.  For each
	* object file, jump to its symbol table and check each external
	* symbol for a match.  If found, return the date of the module
	* containing the symbol.
	*/
	if (fseek(arfd, sizeof(short), 0))
seek_error:	fatal1(":282:seek error on archive %s", an);

	while (fread((char *) & ar_old, sizeof(ar_old), 1, arfd) == 1) {
		last = ftell(arfd);
		if (ar_old.ar_size < sizeof(arobj_old) || 
		    fread((char *) & arobj_old, sizeof(arobj_old),
			  1, arfd) != 1 || 
		    (arobj_old.a_magic != 0401 && 	/* A_MAGIC0 */
		     arobj_old.a_magic != 0407 && 	/* A_MAGIC1 */
		     arobj_old.a_magic != 0410 && 	/* A_MAGIC2 */
		     arobj_old.a_magic != 0411 && 	/* A_MAGIC3 */
		     arobj_old.a_magic != 0405 && 	/* A_MAGIC4 */
		     arobj_old.a_magic != 0437)) 	/* A_MAGIC5 */
			fatal1(":291:%s is not an object module (bu42)",
				ar_old.ar_name);

		skip = arobj_old.a_text + arobj_old.a_data;
		if (!arobj_old.a_flag)
			skip *= 2L;
		if (skip >= ar_old.ar_size || fseek(arfd, skip, 1))
			goto seek_error;
		skip = ar_old.ar_size;
		skip += (skip & 01) + last;
		i = (arobj_old.a_syms / sizeof(ars_old)) + 1;
		while ( --i ) {	/* look through symbol table */
			if (fread((char *) &ars_old, sizeof(ars_old), 1, arfd) != 1)
				fatal1(":285:read error on archive %s",
					an);

			if ((ars_old.n_type & 040)	/* N_EXT for old type */
			     && STREQN(ars_old.n_name, name, (int) len)) {
				(void)strncpy(archmem, ar_old.ar_name, (size_t)14);
				archmem[14] = CNULL;
				return (ar_old.ar_date);
			}
		}
		if (fseek(arfd, skip, 0))
			goto seek_error;
	}
	return (0L);
}


static int
openarch(f)
register CHARSTAR f;
{
	long	sgetl(), ftell();
	extern	int	errno;
	unsigned short	mag_old;	/* old archive format */
	char	mag_5[SAR5MAG],		/* 5.0 archive format */
		mag_port[SARportMAG];	/* port (6.0) archive format */

	
	if ( !(arfd = fopen(f, "r")) ){
		if(errno == ENOENT )
			return(-1);
		else
			fatal1(":262:cannot open %s for reading\n", f);
	}
	/*
	* More code for three archive formats.  Read in just enough to
	* distinguish the three types and set ar_type.  Then if it is
	* one of the newer archive formats, gather more info.
	*/
	if (fread((char *) & mag_old, sizeof(mag_old), 1, arfd) != 1)
		fatal1(":258:%s is not an archive", f);
	if (mag_old == (unsigned short)ARoldMAG) {
		ar_type = ARold;
		first_ar_mem = ftell(arfd);
		sym_begin = num_symbols = sym_size = 0L;
		return (0);
	}
	if (fseek(arfd, 0L, 0) || fread(mag_5, SAR5MAG, 1, arfd) != 1)
		fatal1(":258:%s is not an archive", f);
	if (STREQN(mag_5, AR5MAG, SAR5MAG)) {
		ar_type = AR5;

		/* Must read in header to set necessary info */

		if (fseek(arfd, 0L, 0) || 
		    fread((char *) & arh_5, sizeof(arh_5), 1, arfd) != 1)
			return (-1);

		sym_begin = ftell(arfd);
		num_symbols = sgetl(arh_5.ar_syms);
		first_ar_mem = sym_begin + sizeof(ars_5) * num_symbols;
		sym_size = 0L;
		return (0);
	}
	if (fseek(arfd, 0L, 0) ||
	    fread(mag_port, SARportMAG, 1, arfd) != 1)
				fatal1(":258:%s is not an archive", f);

	if (STREQN(mag_port, ARportMAG, SARportMAG)) {
		ar_type = ARport;
		/*
		* Must read in first member header to find out
		* if there is a symbol directory
		*/
		if (fread((char *) & ar_port,
			  sizeof(ar_port), 1, arfd) != 1 || 
		    !STREQN(ARFportMAG, ar_port.ar_fmag,
		   	    sizeof(ar_port.ar_fmag)))
			return (-2);

		if (ar_port.ar_name[0] == '/') {
			char	s[4];

			if (sscanf(ar_port.ar_size, "%ld", &sym_size) != 1)
				return (-2);
			sym_size += (sym_size & 01);	/* round up */
			if (fread(s, sizeof(s), 1, arfd) != 1)
				return (-2);
			num_symbols = sgetl(s);
			sym_begin = ftell(arfd);
			first_ar_mem = sym_begin + sym_size - sizeof(s);
		} else {
			/* there is no symbol directory */
			sym_size = num_symbols = sym_begin = 0L;
			first_ar_mem = ftell(arfd) - sizeof(ar_port);
		}
		return (0);
	}
	fatal1(":259:%s is unknown archive", f);
}


static DIR *
getfid(dirname)
char *dirname;
{
	char	temp[MAXPATHLEN];
	register DIR * dirf;
	register OPENDIR fod, od = firstod;
	OPENDIR cod, odpred = NULL;
	void cat();

	cat(temp, dirname, 0);
	(void) compath(temp);

	while ( od ) {
		if (STREQ(temp, od->dirn)) {
			if ( !(dirf = od->dirfc) ) {
				if (nopen >= MAXOPFIL) {
					for (fod = firstod; fod; fod = fod->nextopendir)
						if ( fod->dirfc )
							cod = fod;
					(void)closedir (cod->dirfc);
					cod->dirfc = NULL;
					nopen--;
				}
				if ( !(dirf = opendir(od->dirn)) )
					return((DIR * ) -1);
				else {
					od->dirfc = dirf;
					nopen++;
				}
			} else	/* start over at the beginning  */
				(void)rewinddir(dirf);

			if (odpred) {
				odpred->nextopendir = od->nextopendir;
				od->nextopendir = firstod;
				firstod = od;
			}
			return(dirf);
		}

		odpred = od;
		od = od->nextopendir;
	}

	od = ALLOC(s_opendir);
	od->nextopendir = firstod;
	firstod = od;
	od->dirn = copys(temp);
	if (nopen >= MAXOPFIL) {
		for (fod = firstod; fod; fod = fod->nextopendir)
			if ( fod->dirfc )
				cod = fod;
		(void)closedir(cod->dirfc);
		cod->dirfc = NULL;
		nopen--;
	}
	
	if ( dirf = opendir(temp) )
		nopen++;

	od->dirfc = dirf;

	return(dirf);
}

/*
 * The intent here is to provide a means to make the value of
 * bytes in an io-buffer correspond to the value of a long
 * in the memory while doing the io a `long' at a time.
 * Files written and read in this way are machine-independent.
 *
 */
#ifdef __STDC__
# include <limits.h>
#else
# include <values.h>
# define CHAR_BIT BITSPERBYTE
# define const
#endif

long
sgetl(buffer)
register const char *buffer;
{
 	register long w = 0;
	register int i = CHAR_BIT * sizeof(long);

	while ((i -= CHAR_BIT) >= 0)
		w |= (long) ((unsigned char) *buffer++) << i;
	return (w);
}
