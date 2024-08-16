/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtupgrade:dtcdb_scan.c	1.2"

/******************************file*header********************************
 *
 * dtcdb_scan.c - scan the following old cdb files from a given directory,
 *	and generate the corresponding usr.cdb file.
 *		system		-> usr_system.cdb
 *		dtadmin		-> usr_dtadmin.cdb
 *		symtem.post	-> usr_system_post.cdb
 *
 * Diagnostics - although it is not normal, the implementation won't treat
 *	the following situation as a fatal error:
 *
 *		`system', `dtadmin', and/or `system.post' either can be
 *		zero length file(s) or don't exist.
 *
 *	The program is terminated if any of the following situation(s) exist:
 *
 *		`known_classes' file doesn't exist or is a zero length file;
 *		`known_cdbs' file doesn't exist or is a zero lenght file;
 *		can't open `this_dir'/usr_system.cdb for writing;
 *		can't open `this_dir'/usr_dtadmin.cdb for writing;
 *		can't open `this_dir'/usr_system_post.cdb for writing.
 *
 * Logic - the program will scan system, dtadmin, system.post, and any
 *	cdb file(s) that appear in the file (via INCLUDE) if the cdb file
 *	is a known cdb file (via known_cdbs).
 *
 *	The program will place any unknown class(es) (via known_classes)
 *	into corresponding usr*_cdb file, e.g., if we found a new class
 *	in `system' or any known cdb file (via known_cdbs) that is included
 *	in `system' directly or indirectly, then this new class will be
 *	included in `usr_system.cdb'.
 *
 *	The same applies to any unknown cdb file (via INCLUDE), i.e.,
 *	if we found a new cdb file in `system' or any known cdb file
 *	(via known_cdbs) that is included in `system' directly or
 *	indirectly, then this new cdb file will be included in
 *	`usr_system.cdb'.
 *
 * Note that a zero length usr_*.cdb will be created if there is
 *	no new class name and no new INCLUDE line in the given file.
 *
 * Usage - dtcdb_scan <this_dir> <known_classes> <known_cdbs> [cdb_file...]
 *	this_dir	the directory to scan
 *	known_classes	a file contains any known class names
 *	known_cdbs	a file contains any known cdb files.
 *	cdb_file	this is optional and is a hook/hidden feature.
 *			If we need to scan other known cdb files but
 *			they do not appear in system, dtadmin, system.post
 *			files (i.e., via INCLUDE). All new classes and
 *			new INCLUDE lines will go to usr_system_post.cdb
 *			if this feaure is used!
 *
 *	Output -
 *		usr_system.cdb, usr_dtadmin.cdb, and usr_system_post.cdb
 *		will be created in <this_dir>.
 *
 * Return value -
 *	This a.out returns 0 if sucessful, otherwise, it will return 1,
 *	(See Diagnostics above for reasons).
 */

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

/* Declare any #define's here */

#define NUM_KEYWORDS	((int)(sizeof(keywords) / sizeof(keywords[0])))

#define ThisF(f)	glob_data.f

#define WithThisDir	1
#define WithoutThisDir	0

#define ADM		"dtadmin"
#define POST		"system.post"
#define SYS		"system"
#define USR_ADM		"usr_dtadmin.cdb"
#define USR_POST	"usr_system_post.cdb"
#define USR_SYS		"usr_system.cdb"

#define MF_EOFPTR(MP)	((MP)->end_ptr)
#define MF_CURPTR(MP)	((MP)->cur_ptr)
#define MF_EOF(MP)	((MP)->cur_ptr == (MP)->end_ptr)
#define MF_NOT_EOF(MP)	((MP)->cur_ptr != (MP)->end_ptr)
#define MF_NEWLINE(MP)	((*((MP)->cur_ptr) == '\n') ? (MP)->line++ : 0)
#define MF_PEEKC(MP)	(MF_NOT_EOF(MP) ? *((MP)->cur_ptr) : EOF)
#define MF_GETC(MP)	(MF_NOT_EOF(MP) ? (MF_NEWLINE(MP),*(((MP)->cur_ptr)++))\
			 : EOF)
#define MF_GETPTR(MP)	(MF_NOT_EOF(MP) ? ((MP)->cur_ptr) : NULL)
#define MF_NEXTC(MP)	(MF_NOT_EOF(MP) ? (MF_NEWLINE(MP), (MP)->cur_ptr++) : 0)
#define MF_LINENO(MP)	((MP)->line)

#define TOKEN_EOF	-1
#define TOKEN_STRING	-2
#define TOKEN_SEMICOLON	-3
#define TOKEN_MISC	-4
#define TOKEN_CLASS	0
#define TOKEN_INCLUDE	1
#define TOKEN_END	2

/* Declare any data structures here */

typedef struct {
	const char * const	key;
	const unsigned long	len;
} KeyWordRec, *KeyWordPtr;

typedef struct {
	char *			name;
	long			len;
} TokenRec, *TokenPtr;

typedef struct {
	size_t		file_size;
	int		line;		/* current line count */
	char *		map_ptr;	/* ptr to mapped space */
	char *		cur_ptr;	/* ptr to current location */
	char *		end_ptr;	/* ptr to the byte after the
					 * end of mapped space */
} MapFileRec, *MapFilePtr;

typedef struct {			/* Use ThisF to access the field */
	char *		this_dir;
	MapFilePtr	known_classes;
	MapFilePtr	known_cdbs;
	unsigned long *	class_offset_table;
	unsigned long *	cdb_offset_table;
	char **		includes;
	unsigned short	size_includes;
	unsigned short	num_includes;
} GlobalData;

/* Declare forward procedure/function definitions here */

static int *		CalcOffsets(MapFilePtr, unsigned long **);
static int		DidThisInclude(char *);
static char *		FindThisChar(MapFilePtr, int);
static int		FindThisToken(TokenPtr, MapFilePtr, unsigned long *);
static int		GetToken(TokenPtr, MapFilePtr, char *);
static void		KeepThis(int, MapFilePtr, FILE *, char *, char *);
static MapFilePtr	MapFile(char *, int);
static void		ParseThisFile(MapFilePtr, FILE *, char *, char *);
static void		RecordThisInclude(char *);
static int		Strnicmp(register const char *, register const char *,
					register int);

/* Declare static/global variables here */

static KeyWordRec keywords[] = {
	{ "CLASS",		5 },	/* TOKEN_CLASS */
	{ "INCLUDE",		7 },	/* TOKEN_INCLUDE */
	{ "END",		3 },	/* TOKEN_END */
	{ NULL,			0 },	/* last */
};

static GlobalData	glob_data;

/*
 * MapFile - mmap a readonly file.
 *
 * Note that no checking is done!
 */
static MapFilePtr
MapFile(char * name, int with_this_dir)
{
	char		buff[1024];
	char *		file_name;

	char *		fp;
	int		fd;
	int		flag;
	struct stat	st;
	MapFilePtr	mp;

	if (with_this_dir != WithThisDir)
		file_name = name;
	else
	{
		sprintf(buff, "%s/%s", ThisF(this_dir), name);
		file_name = buff;
	}

	if ((fd = open(file_name, O_RDONLY)) == -1)
	{
		fprintf(stderr, "MapFile - can't open %s for reading\n",
							file_name);
		return(NULL);
	}

	if ((flag = fstat(fd, &st) == -1) || st.st_size == 0 ||
	    (fp = (char *)mmap((caddr_t)0, st.st_size,
				PROT_READ, MAP_PRIVATE, fd, 0)) == (char *)-1)
	{
		if (flag == -1)
			fprintf(stderr, "MapFile - can't fstat %s (errno=%d)\n",
							file_name, errno);
		else if (fp == (caddr_t)-1)
			fprintf(stderr, "MapFile - can't mmap %s (errno=%d)\n",
							file_name, errno);
		/* else it's just a zero length file */

		(void)close(fd);
		return(NULL);
	}

	if ((mp = (MapFilePtr)malloc(sizeof(MapFileRec))) == NULL)
	{
		fprintf(stderr, "MapFile - Out of memory\n");
		exit(2);
	}

	mp->file_size = st.st_size;
	mp->end_ptr = fp + st.st_size;
	mp->map_ptr =
	mp->cur_ptr = fp;
	mp->line = 1;
	return(mp);

} /* end of MapFile */

/*
 * FindThisChar - Look for a specific character, starting from
 *			the current position.
 */
static char *
FindThisChar(MapFilePtr mp, int ch)
{
	while (MF_NOT_EOF(mp) && (MF_PEEKC(mp) != ch))
		MF_NEXTC(mp);

	return(MF_GETPTR(mp));
} /* end of FindThisChar */

/*
 * Strnicmp - case insensive version of strncmp.
 */
static int
Strnicmp(register const char * str1, register const char * str2,
							register int len)
{
	register int	c1;
	register int	c2;

	while ((--len >= 0) &&
		((c1 = toupper(*str1)) == (c2 = toupper(*str2++))))
		if (*str1++ == '\0')
			return(0);
	return(len < 0 ? 0 : (c1 - c2));
} /* end of Strnicmp */

/*
 * GetToken -
 */
static int
GetToken(TokenPtr token, MapFilePtr mp, char * cdb)
{
	register int	c;
	char *		save = NULL;
	char *		nextp;
	int		match_this;

again:
		/* skip white spaces */
	while (isspace(MF_PEEKC(mp)) || MF_PEEKC(mp) == '\n')
		MF_NEXTC(mp);

	if (MF_EOF(mp))
		return(TOKEN_EOF);

	c = MF_PEEKC(mp);

	match_this = 0;
	switch(c) {

	case '\'':
		match_this = '\'';
		/* FALLS THROUGH */
	case '"':
		if (!match_this)
			match_this = '"';

			/* get a quoted string */
		MF_NEXTC(mp);
		save = MF_GETPTR(mp);
		FindThisChar(mp, match_this);
		if (MF_GETPTR(mp) == NULL) {
			fprintf(stderr,
				"GetToken - error in %s, can't match %c\n",
								cdb);
			return(TOKEN_EOF);
		}
		token->name = save;
		token->len = MF_GETPTR(mp) - save;
		MF_NEXTC(mp);
		return(TOKEN_STRING);
	case ';':
		MF_NEXTC(mp);
		return(TOKEN_SEMICOLON);
	case ',':
		MF_NEXTC(mp);
		return(TOKEN_MISC);
	case '#':
			/* comment */
		FindThisChar(mp, '\n');
		goto again;
	default:
		{
			/* is a keyword? */
		KeyWordPtr	kp = keywords;
		int		len;
	
		save = MF_GETPTR(mp);
		while (MF_NOT_EOF(mp) &&
		       !strchr(",;\"", MF_PEEKC(mp)) &&
		       !isspace(MF_PEEKC(mp)))
				MF_NEXTC(mp);
		len = MF_GETPTR(mp) - save;
	
		/* lookup a keyword? */
		while (kp->key) {
			if ((len == kp->len) &&
			    !Strnicmp(kp->key, save, len)) {
				return(kp - keywords);
			}
			kp++;
		}

			/* not a keyword, so just say it is a string */
		token->name = save;
		token->len = len;
		return(TOKEN_STRING);
		} /* default */
	}
} /* end of GetToken */

/*
 * ParseThisFile -
 */
static void
ParseThisFile(MapFilePtr sys, FILE * usr, char * cdb_name, char * usr_cdb_name)
{
#define FindThis()	FindThisToken(&token, this_known, this_offsets)
#define GetThis() 	GetToken(&token, sys, cdb_name)
#define SetErrorCode()	if (op == TOKEN_CLASS) \
		unknown_classes = unknown_classes ? unknown_classes * -1 : -1;\
	else	unknown_cdbs    = unknown_cdbs    ? unknown_cdbs * -1 : -1
#define SetUnknown()	if (op == TOKEN_CLASS) \
		unknown_classes++; else unknown_cdbs++

	char		buff[256];	/* for INCLUDE `cdb' */
	int		op, level;
	int		unknown_classes = 0, unknown_cdbs = 0;
	MapFilePtr	this_known;
	unsigned long *	this_offsets;
	TokenRec	token;

	while ((op = GetThis()) != TOKEN_EOF)
	{
		switch(op)
		{
		case TOKEN_CLASS:
		case TOKEN_INCLUDE:	/* FALL THROUGH */
			if (GetThis() != TOKEN_STRING)
			{
				fprintf(stderr, "ParseThisFile: can't get %s\n",
						op == TOKEN_CLASS ?
							"class name" :
							"include name 1");
				SetErrorCode();
				break;
			}
			else
			{
				if (op == TOKEN_CLASS)
				{
					this_known = ThisF(known_classes);
					this_offsets =ThisF(class_offset_table);
				}
				else if (op == TOKEN_INCLUDE)
				{
					if (GetThis() != TOKEN_SEMICOLON)
					{
						fprintf(stderr, "ParseThisFile:\
 can't get include name 2\n");
						SetErrorCode();
						break;
					}
					this_known = ThisF(known_cdbs);
					this_offsets =ThisF(cdb_offset_table);
				}

				strncpy(buff, token.name, token.len);
				buff[token.len] = 0;

	/* Should we check `class' name also?
	 * The current implementation didn't check!
	 */
				if (op == TOKEN_INCLUDE)
				{
					if (DidThisInclude(buff))
					{
						fprintf(stderr,"ParseThisFile:\
 already included/parsed this include name %s\n", buff);
						continue;
					}
					RecordThisInclude(buff);
				}

				if (!FindThis())
				{
	fprintf(stderr, "Save this %s %s from %s/%s to %s/%s\n",
		buff, op == TOKEN_CLASS ? "CLASS" : "INCLUDE",
		ThisF(this_dir), cdb_name, ThisF(this_dir), usr_cdb_name);

					SetUnknown();
					KeepThis(op, sys, usr, cdb_name, buff);
				}
				else if (op == TOKEN_INCLUDE)
				{
					MapFilePtr	this_sys;

					if (this_sys=MapFile(buff, WithThisDir))
						ParseThisFile(this_sys, usr,
							buff, usr_cdb_name);
				}
			}
			break;
		default:
			/* do nothing here */
			break;
		}
	}

	munmap(sys->map_ptr, sys->file_size);
	printf("%s %d %d\n", cdb_name, unknown_classes, unknown_cdbs);

#undef FindThis
#undef GetThis
#undef SetErrorCode
#undef SetUnknown
} /* end of ParseThisFile */

/*
 * FindThisToken - find `key' from `mp' based on `offsets' table.
 */
static int
FindThisToken(TokenPtr key, MapFilePtr mp, unsigned long * offsets)
{
#define I	offsets[i]
#define LEN	(offsets[i+1] - offsets[i] - 1)
#define NAME	&(mp)->map_ptr[I]

	register int	i;
	int		got_it = 0;

	if (mp == NULL)		/* known* can be NULL */
		return(got_it);

	for (i = 0; i < mp->line - 1; i++)
	{
		if (key->len == LEN && strncmp(NAME, key->name, LEN) == 0)
		{
			got_it = 1;
			break;
		}
		else{
		    char *end_known;
		    char *name;
		    char *name_end;

			/* look for trailing '*' at end of "known" name */
		    
		    end_known = NAME + LEN-1;
		    if (*end_known == '*' && strncmp(NAME, key->name, LEN-1) == 0){
				name = key->name + LEN - 1;
				name_end = key->name + key->len;
				while(name < name_end && isdigit(*name))
					name++;
				if (name == name_end){
			    	got_it = 1;
			    	break;
				}
		    }
		}
	}

	return(got_it);

#undef I
#undef LEN
#undef NAME
} /* end of FindThisToken */

static void
KeepThis(int op, MapFilePtr sys, FILE * usr, char * cdb_name, char * name)
{
#define GetThis()	GetToken(&token, sys, cdb_name)

	static int		prev_op = 999;
	static MapFilePtr	prev_sys = NULL;
	static FILE *		prev_usr = NULL;

	char *		this_ptr;
	TokenRec	token;

#if 0
	fprintf(usr, "# The %s (%s) below was from\n",
		op == TOKEN_CLASS ? "CLASS block" : "INCLUDE line", name);
	fprintf(usr, "# \t%s/%s originally!\n#\n", ThisF(this_dir), cdb_name);
#else
	if (op != prev_op || sys != prev_sys || usr != prev_usr)
	{
		prev_op  = op;
		prev_sys = sys;
		prev_usr = usr;

		fprintf(usr, "# The %s below were from ",
			op == TOKEN_CLASS ? "CLASS blocks" :
					    "INCLUDE lines");
		fprintf(usr, "%s/%s originally!\n#\n",
			ThisF(this_dir), cdb_name);
	}
#endif

	if (op == TOKEN_INCLUDE)
	{
		fprintf(usr, "INCLUDE %s;\n", name);
		return;
	}
/* TOKEN_CLASS */

	this_ptr = sys->cur_ptr;
	fprintf(usr, "CLASS %s", name);

	while ((op = GetThis()) != TOKEN_EOF && op != TOKEN_END);

	if (op == TOKEN_END)	/* should we assume always see this? */
	{
		while (this_ptr != sys->cur_ptr)
		{
			putc(*this_ptr, usr);	/* use marco for speed */
			this_ptr++;
		}
		putc('\n', usr);
		putc('\n', usr);
	}

#undef GetThis
} /* end of KeepThis */

/*
 * RecordThisInclude -
 */
static void
RecordThisInclude(char * this_include)
{
#define ADD_THIS_MUCH	5

	if (ThisF(num_includes) == ThisF(size_includes))
	{
		ThisF(size_includes) += ADD_THIS_MUCH;
		ThisF(includes) = (char **)realloc(ThisF(includes),
					ThisF(size_includes) * sizeof(char *));
	}

	ThisF(includes)[ThisF(num_includes)] = strdup(this_include);
	ThisF(num_includes)++;

#undef ADD_THIS_MUCH
} /* end of RecordThisInclude */

/*
 * DidThisInclude -
 */
static int
DidThisInclude(char * this_include)
{
	register int	i;

	for (i = 0; i < (int)ThisF(num_includes); i++)
		if (!strcmp(this_include, ThisF(includes)[i]))
			return(1);

	return(0);
} /* end of DidThisInclude */

/*
 * CalcOffsets -
 */
static int *
CalcOffsets(MapFilePtr mp, unsigned long ** offsets)
{
	char *				prev_ptr;
	unsigned long			prev;
	register unsigned long *	this_loc;

/* First pass - try to get number of lines */
	while(1)
	{
		FindThisChar(mp, '\n');
		if (MF_NOT_EOF(mp))
			MF_NEXTC(mp);
		else
			break;
	}

	*offsets = this_loc = (unsigned long *)malloc(
					sizeof(unsigned long) * (mp->line+1));

/* Initializing... */
	prev = 0;
	mp->cur_ptr = prev_ptr = mp->map_ptr;
	mp->line = 1;
	*this_loc = 0;
	this_loc++;

/* Second pass, figure out the answer now */

	while(1)
	{
		FindThisChar(mp, '\n');
		*this_loc = mp->cur_ptr - prev_ptr + prev + 1;
		prev = *this_loc;
		prev_ptr = mp->cur_ptr + 1;	/* new line */
		this_loc++;
		if (MF_NOT_EOF(mp))
			MF_NEXTC(mp);
		else
			break;
	}
} /* end of CalcOffsets */

int
main(int argc, char ** argv)
{
	char		ua_buff[1204];
	char		up_buff[1204];
	char		us_buff[1204];
	int		i;
	FILE *		usr_adm;
	FILE *		usr_post;
	FILE *		usr_sys;
	MapFilePtr	adm;
	MapFilePtr	post;
	MapFilePtr	sys;
	MapFilePtr	misc;

	if (argc < 4)
	{
		fprintf(stderr,
			"Usage: %s <this_dir> <known_classes> <known_cdbs>\n",
								argv[0]);
		exit(3);
	}

	ThisF(this_dir) = strdup(argv[1]);

#define INITIAL_INCLUDE_ENTRIES		50
	ThisF(includes) = (char **)malloc(INITIAL_INCLUDE_ENTRIES *
							sizeof(char *));
	ThisF(size_includes) = INITIAL_INCLUDE_ENTRIES;
	ThisF(num_includes) = 0;

	RecordThisInclude(SYS);
	RecordThisInclude(ADM);
	RecordThisInclude(POST);
#undef INITIAL_INCLUDE_ENTRIES

	sprintf(ua_buff, "%s/%s", ThisF(this_dir), USR_ADM);
	sprintf(up_buff, "%s/%s", ThisF(this_dir), USR_POST);
	sprintf(us_buff, "%s/%s", ThisF(this_dir), USR_SYS);
	if (!(ThisF(known_classes) = MapFile(argv[2], WithoutThisDir)) ||
	    !(ThisF(known_cdbs) = MapFile(argv[3], WithoutThisDir)) ||
	    !(usr_adm = fopen(ua_buff, "w")) ||
	    !(usr_post = fopen(up_buff, "w")) ||
	    !(usr_sys = fopen(us_buff, "w")))
	{
			/* no need to close anything... */
		return(1);
	}

	CalcOffsets(ThisF(known_classes), &ThisF(class_offset_table));
	CalcOffsets(ThisF(known_cdbs), &ThisF(cdb_offset_table));

	if (sys = MapFile(SYS, WithThisDir))
		ParseThisFile(sys, usr_sys, SYS, USR_SYS);

	if (adm = MapFile(ADM, WithThisDir))
		ParseThisFile(adm, usr_adm, ADM, USR_ADM);

	if (post = MapFile(POST, WithThisDir))
		ParseThisFile(post, usr_post, POST, USR_POST);

	for (i = 4; i < argc; i++)
	{
		if (DidThisInclude(argv[i]))
		{
			fprintf(stderr, "main:\
 already included/parsed this include name %s\n", argv[i]);
			continue;
		}
		RecordThisInclude(argv[i]);
		if (misc = MapFile(argv[i], WithThisDir))
			ParseThisFile(misc, usr_post, argv[i], USR_POST);
	}

	fclose(usr_adm);
	fclose(usr_post);
	fclose(usr_sys);

	munmap(ThisF(known_classes)->map_ptr, ThisF(known_classes)->file_size);
	munmap(ThisF(known_cdbs)->map_ptr, ThisF(known_cdbs)->file_size);

	return(0);
} /* end of main */
