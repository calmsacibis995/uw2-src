/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)skey:keyadm.c	1.11"
#ident  "$Header: $"

#include <stdio.h>
/*
#include <stdlib.h>
*/
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <locale.h>
#include <pfmt.h>
#include <unistd.h>
#include <sys/keyctl.h>
#include <sys/mac.h>
#include <sys/stat.h>

static int	parseline(uchar_t *, k_skey_t *);
static int	loadkeys(int, char *);
static int	addkeystofile(char *);
static int	writetofile(FILE *, char *);
static void	usage(void);

static struct resrc {
        char *name;
        int  resource;
	char *msg;
} resrctab[] = {
        {"USERS", 	K_USER, ":49"},
        {"PROCESSORS",	K_PROC, ":50"}
};

#define VAL_NRESOURCE  (sizeof resrctab / sizeof (struct resrc))


static char *dflt_file = "/etc/config/licensekeys";

int
main(int argc, char *argv[])
{
	extern char	*optarg;
	boolean_t	addkeysflag = B_FALSE;
	boolean_t	setlimitflag = B_FALSE;
	boolean_t	getlimitflag = B_FALSE;
	boolean_t	listflag = B_FALSE;
	boolean_t	allresrc_flag;
	int		nflags = 0;
	char		*filename = NULL;
	int		nresource;
	int		resource;
	char 		*resrcmsg;
	char		*resourcename;
	int		resrclimit;
	int		i, j, c;
	int		ret = 0;

        (void)setlocale(LC_ALL, "");
        (void)setcat("uxmp");
	(void)setlabel("UX:keyadm");

	while ((c = getopt(argc, argv, "f:asgl")) != EOF) {
		switch (c) {
			case 'f':
				filename = optarg;
				break;
			case 'a':
				addkeysflag = B_TRUE;
				nflags++;
				break;
			case 's':
				setlimitflag = B_TRUE;
				nflags++;
				break;
			case 'g':
				getlimitflag = B_TRUE;
				nflags++;
				break;
			case 'l':
				listflag = B_TRUE;
				nflags++;
				break;
			case '?':
			default:
				pfmt(stderr, MM_ERROR, ":51:Incorrect usage\n");
				usage();
				return 1;
		}
	}
	/*
	 * One, and only one, of the four flags "a, s, g, l" must be specified.
	 */
	if (nflags != 1) {
		if (nflags > 1)
			pfmt(stderr, MM_ERROR,
			     ":52:invalid combination of options\n");
		usage();
		return 1;
	}

	/*
	 * The filename option is ilegal with the flags `l' or `g'.
	 */
	if ((listflag || getlimitflag) && filename != NULL) {
		pfmt(stderr, MM_ERROR, ":52:invalid combination of options\n");
		usage();
		return 1;
	}

	/*
	 * For the flags `s' and `a' use the system default file if the
	 * filename is not supplied.
	 */
	if (addkeysflag || setlimitflag) {
		if (filename == NULL)
			filename = dflt_file;
	}

	if (listflag) {
		/*
		 * The `l' flag requires no arguments.
		 */
		if (argc != optind) {
			pfmt(stderr, MM_ERROR, ":51:Incorrect usage\n");
			usage();
			return 1;
		} else {
			/*
			 * List all valid resource names
			 */
			for (i = 0; i < VAL_NRESOURCE; i++)
				pfmt(stdout, MM_NOSTD|MM_NOGET,"%s\n",
				     gettxt(resrctab[i].msg, resrctab[i].name));
			return 0;
		}
	}

	if (addkeysflag) {
		/*
		 * The `a' flag requires no arguments.
		 */
		if (argc != optind) {
			pfmt(stderr, MM_ERROR, ":51:Incorrect usage\n");
			usage();
			return 1;
		} else {
			/*
			 * Add key pairs to filename 
			 */ 
			return addkeystofile(filename);
		}
	}

	if (setlimitflag)
		/*
		 * Call loadkeys() to set limit for all valid resources.
		 */
		return loadkeys(K_SETLIMIT, filename);

	/*
	 * For flag `g' determine if at least one resource name
	 * is supplied. If no resource name supplied, get all valid
	 * resources.
	 */
	if (argc > optind) {
		nresource = argc - optind;
		allresrc_flag = B_FALSE;
	} else {
		nresource = VAL_NRESOURCE;
		allresrc_flag = B_TRUE;
	}

	for (i = 0; i < nresource; i++) {
		if (allresrc_flag) {
			resource = resrctab[i].resource;
			resourcename = resrctab[i].name;
			resrcmsg = resrctab[i].msg;
		} else {
			resourcename = argv[optind++];
			for (resource = -1, j = 0; j < VAL_NRESOURCE; j++) {
			      if (strcmp(resourcename, resrctab[j].name) == 0) {
					resource = resrctab[j].resource;
					resrcmsg = resrctab[j].msg;
					break;
			      }
			}
			if (resource == -1) {
				pfmt(stderr, MM_ERROR,
				     ":53:\n\t%s is an invalid resource\n",
				     resourcename);
				ret = 1;
				continue;
			}
		}

		/*
		 * get and display the resource limit
		 */
		if (getlimitflag) {
			resrclimit = keyctl((K_GETLIMIT | resource), NULL, 0);
			if (resrclimit == K_UNLIMITED)
				pfmt(stdout, MM_NOSTD,
				     ":54:UNLIMITED\t%s\n",
				     gettxt(resrcmsg, resourcename));
			else
				pfmt(stdout, MM_NOSTD|MM_NOGET,
				    "%d\t%s\n",
				    resrclimit, gettxt(resrcmsg, resourcename));

		}
		ret = 0;
	}
	return ret;
}

void
usage(void)
{
	static char usage1[] = "keyadm -a [-f filename]";
	static char usage2[] = "keyadm -g [resource ...]";
	static char usage3[] = "keyadm -s [-f filename]";
	static char usage4[] = "keyadm -l";
	pfmt(stderr, MM_ACTION, ":55:Usage:\n\t%s\n\t%s\n\t%s\n\t%s\n",
	     gettxt(":56", usage1), gettxt(":57", usage2),
	     gettxt(":58", usage3), gettxt(":59", usage4));
}


int
loadkeys(int cmd, char *filename)
{
	k_skey_t skeys[MAXSKEYS]; 
	char	line[BUFSIZ];
	char	*errmsg;
	char	*strerr;
	FILE	*fp;
	int	i = 0;
	int	error;

	if ((fp = fopen(filename, "r")) == NULL) {
		error = errno;
		if (strcmp(filename, dflt_file) == 0)
			pfmt(stderr, MM_ERROR,
				":60:\n\tUnable to open database file: %s\n",
				strerror(error));
		else
			pfmt(stderr, MM_ERROR,
				":61:\n\tUnable to open %s: %s\n",
				filename, strerror(error));
		return error;
	}

	while (fgets(line, BUFSIZ, fp) != NULL && i < MAXSKEYS) {
		i += parseline((uchar_t *)line, &skeys[i]);
	}

	if (keyctl(cmd, skeys, i) == -1) {
		error = errno;
		errmsg = gettxt(":62","Error setting limit.");

		if (error == EEXIST)
			strerr = gettxt(":63","Serial number duplicated");
		else if (error == ETIME)
                        strerr = gettxt(":64",
					"Not enough time since last update");
		else if (error == EINVAL)
			strerr = gettxt(":65","Invalid serial/key pair");
		else
			strerr = strerror(error);

		pfmt(stderr, MM_ERROR|MM_NOGET, "\n\t%s %s\n", errmsg, strerr);
		return error;
	}
	return 0;
}

#define	SYS_PRIVATE	2

int
addkeystofile(char *filename)
{
	FILE	*fi = stdin;
	FILE	*fo;
	int	fd;
	struct stat buf;
	char	in_line[BUFSIZ];
	char	key[SKLEN * 2];
	char    *ws = " \t\n";
	char	*ptr;
	char 	*p;
	int	error;
	int	i;
	int	c;
	lid_t	level = SYS_PRIVATE;
	struct prompt {
		char *string;
		char *msg;
	} promptbl[] = {
		{"Enter the upgrade serial Number: ",	":66"},
		{"Enter the upgrade serial Key: ",	":67"}
	};


	if (strcmp(filename, dflt_file) == 0 && stat(filename, &buf) < 0) {
		error = errno;
 		if (error != ENOENT) {
			pfmt(stderr, MM_ERROR,
			  ":68:\n\tUnable to get status of database file: %s\n",
			     strerror(error));
			return error;
		}
		if((fd = creat(filename, S_IRUSR | S_IWUSR)) == -1) {
			error = errno;
			pfmt(stderr, MM_ERROR,
			     ":69:\n\tUnable to create database file: %s\n",
			     strerror(error));
			return error;
		}

		(void)lvlfile(filename, MAC_SET, &level);
	}
	if ((fo = fopen(filename, "a+")) == NULL) {
		error = errno;
		if (strcmp(filename, dflt_file) == 0)
			pfmt(stderr, MM_ERROR,
				":70:\n\tUnable to open database file: %s\n",
				strerror(error));
		else
			pfmt(stderr, MM_ERROR,
				":61:\n\tUnable to open %s: %s\n",
				filename, strerror(error));
		return error;
	}

	p = ttyname(0);

	if (p) {
		/* standard input is the terminal.
		 * request the key pairs from the user
		 * and write them out to "filename".
		 */
		in_line[0] = '\0';
		for (i = 0; i < 2; i++) {
			puts(gettxt(promptbl[i].msg, promptbl[i].string));
			if (fgets(key, sizeof(key), fi) != NULL) {
				if (!strchr(key, '\n')) {
					while ((c = getc(fi)) != '\n' &&
						c != EOF)
						;
				}
				ptr = strtok(key, ws);
				if (ptr != NULL)
					strcat(in_line, ptr);
				else
					strcat(in_line, key);
				strcat(in_line, "\t");

			} else {
				pfmt(stderr, MM_ERROR,
				     ":71:\n\tIncorrect input\n");
				return 1;

			}
		}
		if (writetofile(fo, in_line) != 0)
			return 1;

	} else {
		/* Read the key pairs from standard input and write them
		 * to "filename".
		 */
		while (fgets(in_line, BUFSIZ, fi) != NULL) {
			if (writetofile(fo, in_line) != 0)
				return 1;
		}
	}
	return 0;
}

int
writetofile(FILE *fo, char *in_line)
{
	char	out_line[BUFSIZ];
	char	*ws = " \t\n";
	char	*ptr;
	int	i;
	int	tokenfound = 0;


	for (i = 0, ptr = strtok(in_line, ws); 
		    ptr != NULL && i < 2;
	            ptr = strtok(NULL, ws), i++) {

		tokenfound++;
		if (i == 0) {
			strcpy(out_line, ptr);
			strcat(out_line, "\t");
		} else
			strcat(out_line, ptr);
	}
	if (tokenfound != 2) {
		pfmt(stderr, MM_ERROR, ":71:\n\tIncorrect input\n");
		return 1;
	}
	strcat(out_line, "\n");
	fputs(out_line, fo);
	return 0;
}


int
parseline(uchar_t *buf, k_skey_t *p)
{
	/*
	 * snag the serialnum-id and serialkey out of the line stored in
	 * the file.
	 */
	int	j = 0;
	int	k = 0;
	uchar_t c;

	for ( k = 0; k < STRLEN; k++) {
		p->sernum[k] = '\0';
		p->serkey[k] = '\0';
	}

	/*
	 * eat white space
	 */
	while ((c = buf[j]) == ' ' || c == '\t') { 
		j++;
	}

	/*
	 * return zero if line is a comment of empty.
	 */
	if ((c = buf[j]) == '#' || c == '\n') {
		return 0;
	}

	/*
	 * get the serialnum-id string
	 */
	for (k = 0; k < STRLEN; k++) {
		if ((c = buf[j]) != ' ' && c != '\t' && c != '\n' && c != '\0') 
			p->sernum[k] = buf[j++];
		else 
			break;
	}

	/*
	 * if we haven't seen white space yet, eat chars until
	 * white space is seen.
	 */
	while ((c = buf[j]) != ' ' && c != '\t' && c != '\n' && c != '\0') {
		j++;
	}

	/*
	 * eat white space
	 */
	while ((c = buf[j]) == ' ' || c == '\t') { 
		j++;
	}

	/*
	 * get the serial key string.
	 */
	for (k = 0; k < STRLEN; k++) {
		if ((c = buf[j]) != ' ' && c != '\t' && c != '\n' && c != '\0')
			p->serkey[k] = buf[j++];
		else
			break;
	}
	return 1;
}
