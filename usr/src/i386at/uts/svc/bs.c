/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:svc/bs.c	1.19.1.1"
#ident	"$Header: $"

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

/*
 * Parse the bootstrap arguments and set kernel variables accordingly.
 */

#include <fs/vfs.h>
#include <io/conf.h>
#include <io/conssw.h>
#include <mem/hatstatic.h>
#include <proc/cred.h>
#include <svc/bootinfo.h>
#include <svc/copyright.h>
#include <svc/psm.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#define equal(a,b)	(strcmp(a, b) == 0)

STATIC int bs_doarg(char *);
int bs_lexparms(char *, int *, int);
int bs_lexwords(char *, char *[], int);
char *bs_lexnum(const char *, int *);
char *bs_strchr(const char *, char);
char *bs_stratoi(const char *, int *, int);
int bs_find_bmaj(const char *);
int bs_find_cmaj(const char *);
int bs_lexcon(const char **, conssw_t **, minor_t *, char **);

#define	BASE8	0		/* bases for bs_stratoi */
#define BASE10	1
#define	BASE16	2

/* boot string variables */
#define	MAXPARMS	2		/* number of device parms */

extern char *startupmsg, *rebootmsg, *automsg;
extern char *title[MAXTITLE];
extern uint_t ntitle;
extern boolean_t title_changed;
extern char *copyright[MAXCOPYRIGHT];
extern uint_t ncopyright;
extern boolean_t copyright_changed;
extern char *sdi_devicenames;
extern char *sdi_lunsearch;
extern char *rm_resmgr;
extern boolean_t rm_invoke_dcu;
extern char *initstate;
extern char *pci_scan;
extern char *cm_bootarg[];
extern uint_t cm_bootarg_count;
extern uint_t cm_bootarg_max;


/*
 * void
 * bootarg_parse(void)
 *
 *	Parse the bootstrap string and set rootdev and dumpdev.
 *
 * Calling/Exit State:
 *
 *	Called from the initialization process when the system is
 *	still running on the boot processor.  No return value.
 */
void
bootarg_parse(void)
{
	extern char *kernel_name;
	int i;
	char *s;
	char hold_arg[B_STRSIZ];	/* tmp hold area for bargv[] entry */

	/*
	 * Parse the bootstrap args passed in bootinfo.bargv[].  Note that
	 * bootinfo.bargv[0] contains the name of the kernel booted.
	 * The name of the kernel booted is saved in kernel_name variable.
	 */

	bcopy(&bootinfo.bargv[0][0], hold_arg, B_STRSIZ);
	if (strcmp(hold_arg, kernel_name) != 0) {
		kernel_name = calloc(strlen(hold_arg) + 1);
		strcpy(kernel_name, hold_arg);
	}

	for (i = 1; i < bootinfo.bargc; i++) {	/* parse args */
		bcopy(&bootinfo.bargv[i][0], hold_arg, B_STRSIZ);
		if (*hold_arg)
			(void) bs_doarg(hold_arg);
	}
}

/*
 * STATIC int
 * bs_doarg(char *arg)
 *	Determine argument and further analyze parameters
 *
 * Calling/Exit State:
 *	Returns	-1 if error
 *		 0 if arg correct
 *
 *	Currently, we only care about arguments that have the following form:
 *
 *	  case 1 rootfs[type]=<fstype>
 *	  case 2 TITLE=<msg>			(may be multiple)
 *	  case 3 COPYRIGHT=<msg>		{may be multiple}
 *	  case 4 STARTUPMSG=<msg>
 *	  case 5 REBOOTMSG=<msg>
 *	  case 6 AUTOMSG=<msg>
 * 	  case 7 console=<device>(<minor>[,<paramstr>])
 * 	  case 8 <keyword>=<device>(<parm>[,<parm>])  {root[dev], dump[dev]}
 *        case 9 DEVICENAMES=scsi_device_name,scsi_device_name,...
 *	  case 10 TZ_OFFSET=<seconds>
 *	  case 11 LUNSEARCH=[(C:B,T,L),...]
 *	  case 12 DCU=<YES|NO>
 *	  case 13 RESMGR=<filename>
 *	  case 14 INITSTATE=<str>
 *	  case 15 PCISCAN=<str>
 *	  case 16 resmgr:<modname>:[<instance>:]<param>=<value>
 *
 *	Note that '<>' surround user supplied values; '[]' surround
 *	optional extensions; "parms" are words containing numeric characters;
 *	"devices" are device mnemonics (from bdevsw/cdevsw d_name); and all
 *	other symbols are literal.
 *
 */
STATIC int
bs_doarg(char *s)
{
	int n, i, maj, parms[MAXPARMS];
	char *p;
	char hold_arg[B_STRSIZ];	/* tmp hold area for bargv[] entry */

	bcopy(s, hold_arg, B_STRSIZ);

	/*
	 * case 16: resmgr:<modname>:[<instance>:]<param>=<value> 
	 *
	 * This needs to be parsed before the other arguments because
	 * the keyword is delimited by ':' instead of '='.
	 */
	if (strncmp(s, "resmgr:", 7) == 0) {
		if (cm_bootarg_count >= cm_bootarg_max)
			return -1;
		p = s + 7;
		cm_bootarg[cm_bootarg_count] = calloc(strlen(p) + 1);
		strcpy(cm_bootarg[cm_bootarg_count++], p);
		return 0;			/* arg was handled */
	}

	/* skip over keyword to '=' */
	if ((p = bs_strchr(s, '=')) == (char *)NULL)
		return psm_doarg(hold_arg);

	*p++ = '\0';				/* delimit keyword */

	/* case 1 */
	if (equal(s, "rootfs") || equal(s, "rootfstype")) {
		strncpy(rootfstype, p, ROOTFS_NAMESZ);
		return 0;			/* arg was handled */
	}

	/* case 2 */
	if (equal(s, "TITLE")) {
		if (!title_changed) {
			title_changed = B_TRUE;
			ntitle = 0;
		}
		if (ntitle < MAXTITLE) {
			char *m = calloc(strlen(p) + 1);
			strcpy(m, p);
			title[ntitle++] = m;
		}
		return 0;			/* arg was handled */
	}

	/* case 3 */
	if (equal(s, "COPYRIGHT")) {
		if (!copyright_changed) {
			copyright_changed = B_TRUE;
			ncopyright = 0;
		}
		if (ncopyright < MAXCOPYRIGHT) {
			char *m = calloc(strlen(p) + 1);
			strcpy(m, p);
			copyright[ncopyright++] = m;
		}
		return 0;			/* arg was handled */
	}

	/* case 4 */
	if (equal(s, "STARTUPMSG")) {
		startupmsg = calloc(strlen(p) + 1);
		strcpy(startupmsg, p);
		return 0;			/* arg was handled */
	}

	/* case 5 */
	if (equal(s, "REBOOTMSG")) {
		rebootmsg = calloc(strlen(p) + 1);
		strcpy(rebootmsg, p);
		return 0;			/* arg was handled */
	}

	/* case 6 */
	if (equal(s, "AUTOMSG")) {
		automsg = calloc(strlen(p) + 1);
		strcpy(automsg, p);
		return 0;			/* arg was handled */
	}

	/* case 7: console device override */
	if (equal(s, "console")) {
		return bs_lexcon((const char **)&p,
				 &consswp, &consminor, &consparamp);
	}

	/* case 9: PDI device names */
	if (equal(s, "DEVICENAMES")) {
		sdi_devicenames = calloc(strlen(p) + 1);
		strcpy(sdi_devicenames, p);
		return 0;			/* arg was handled */
	}

	/* case 10: timezone correction */
	if (equal(s, "TZ_OFFSET")) {
		int correction;
		extern time_t c_correct;

		if (*bs_lexnum(p, &correction) == '\0') {
			c_correct = correction;
			return 0;		/* arg was handled */
		}
	}

	/* case 11: PDI limited lun searching */
	if (equal(s, "LUNSEARCH")) {
		sdi_lunsearch = calloc(strlen(p) + 1);
		strcpy(sdi_lunsearch, p);
		return 0;			/* arg was handled */
	}

	/* case 12: DCU=<YES|NO> */

	if (equal(s, "DCU") && (p[0] == 'Y' || p[0] == 'y')) {
		rm_invoke_dcu = B_TRUE;
		return 0;			/* arg was handled */
	}

	/* case 13: RESMGR=<filename> */

	if (equal(s, "RESMGR")) {
		rm_resmgr = calloc(strlen(p) + 1);
		strcpy(rm_resmgr, p);
		return 0;			/* arg was handled */
	}

	/* case 14: INITSTATE=<str> */

	if (equal(s, "INITSTATE")) {
		initstate = calloc(strlen(p) + 1);
		strcpy(initstate, p);
		return 0;			/* arg was handled */
	}

	/* case 15: PCISCAN=<str> */

	if (equal(s, "PCISCAN")) {
		pci_scan = calloc(strlen(p) + 1); /* arg was handled */
		strcpy(pci_scan, p);
		return 0;
	}


	/* case 8: block device overrides */

	n = bs_lexparms(p, parms, MAXPARMS);

						/* look in bdevsw[] */
	if ((n > 0 && n <= MAXPARMS) && ((maj = bs_find_bmaj(p)) != -1)) {

							/* make rootdev */
		if (equal(s, "root") || equal(s, "rootdev")) {
			rootdev = makedevice(maj, parms[0]);
			return 0;			/* arg was handled */
		}
		else					/* make pipedev */
		if (equal(s, "pipe") || equal(s, "pipedev")) {
			; /* pipedev obsolete -- ignored */
			return 0;			/* arg was handled */
		}
		else					/* make swapdev */
		if (equal(s, "swap") || equal(s, "swapdev")) {
			; /* swapdev obsolete -- ignored */
			return 0;			/* arg was handled */
		}
		else					/* make dumpdev */
		if (equal(s, "dump") || equal(s, "dumpdev")) {
			dumpdev = makedevice(maj, parms[0]);
			return 0;			/* arg was handled */
		}
	}

	/* unknown argument */

	return psm_doarg(hold_arg);
}


/*
 * int
 * bs_lexparms(char *s, int *parms, int maxparms)
 *	Extract numeric parameters, delimited by parens, from an argument string
 *
 * Calling/Exit State:
 *		s = input string pointer
 *	    parms = pointer to array of ints
 *	 maxparms = size of parms array
 *
 *	returns	0 = error
 *		n = number of parameters found
 */
int
bs_lexparms(char *s, int *parms, int maxparms)
{
	char *p;
	int n = 0;

	if ((p = bs_strchr(s, '(')) == (char *)0)
		return 0;
	*p = '\0';			/* delimit anything prior to '(' */
	do {
		p++;
		if (maxparms-- == 0)
			return 0;
		p = bs_lexnum(p, &parms[n]);	/* extract number */
		if (p == (char *)0)  		/* no parm error */
			return 0;
		if (*p != ','  &&  *p != ')')
			return 0;
		n++;
	} while (*p == ',');			/* while more parms */
	return n;
}


/*
 * int
 * bs_lexcon(const char **strp, conssw_t **cswp, minor_t *mp, char **paramstrp)
 *	Parse a console device string
 *
 * Calling/Exit State:
 *	Called with no locks held, at console open time.
 *
 * Description:
 *	Parses a string of the form:
 *
 *		<devname>(<minor>[,<paramstr>])
 *
 *	where <devname> is a console-capable device name
 *	      <minor> is the minor number of that device (in decimal)
 *	      <paramstr> is an optional, device-specific, parameter string,
 *			which may include nested, paired parentheses
 *
 *	The input string at *strp does not have to be terminated
 *	at the end of this construct; *strp will be left pointing
 *	to the first character after the closing parenthesis.
 *
 *	*cswp will be set to the conssw_t structure for the named device.
 *
 *	*mp will be set to the value of <minor>.
 *
 *	If paramstrp is non-NULL, *paramstrp will be set to a privately-
 *	allocated copy of <paramstr>.
 *
 *	Returns non-zero if the string was successfully parsed.
 */
int
bs_lexcon(const char **strp, conssw_t **cswp, minor_t *mp, char **paramstrp)
{
	const char *s;
	const char *s2;
	char *p;
	int nest = 0;
	int i, num;

	if ((s = bs_strchr(*strp, '(')) == (char *)0)
		return 0;
	/* find device name in constab */
	for (i = 0;; i++) {
		if (i == conscnt)
			return 0;
		for (s2 = *strp, p = constab[i].cn_name; s2 != s; ++s2, ++p) {
			if (*s2 != *p)
				break;
		}
		if (s2 == s)
			break;
	}
	*cswp = constab[i].cn_consswp;
	++s;
	/* extract minor number */
	if ((s = bs_lexnum(s, &num)) == 0)
		return 0;
	*mp = num;
	/* if no parameter string, return now */
	if (*s != ')' && *s++ != ',')
		return 0;
	/* parse parameter string; find terminating matching paren */
	for (s2 = s, nest = 0; *s2 != ')' || nest != 0; ++s2) {
		if (*s2 == '\0')
			return 0;
		if (*s2 == '(')
			++nest;
		else if (*s2 == ')')
			--nest;
	}
	if (paramstrp != NULL) {
		/* allocate memory for parameter string and copy it */
		p = consmem_alloc(s2 - s + 1, 0);
		if (p == NULL)
			return 0;
		bcopy(s, p, s2 - s);
		p[s2 - s] = '\0';
		*paramstrp = p;
	}
	*strp = s2 + 1;
	return 1;
}


/*
 * int
 * bs_lexwords(char *s, char *parms[], int maxparms)
 *	Extract word parameters, delimited by parens, from an argument string
 *
 * Calling/Exit State:
 *		s = input string pointer
 *	    parms = pointer to array of char *
 *	 maxparms = size of parms array
 *
 *	returns	0 = error
 *		n = number of parameters found
 */
int
bs_lexwords(char *s, char *parms[], int maxparms)
{
	char *p;
	int n = 0;

	if ((p = bs_strchr(s, '(')) == (char *)0)
		return 0;
	do {
		*p = '\0';			/* delimit previous word */
		p++;
		if (maxparms-- == 0)
			return 0;
		parms[n] = p;
		while (*p >= '0' && *p <= '9'  ||  *p >= 'A' &&  *p <= '~')
			p++;
		if (*p != ','  &&  *p != ')')
			return 0;
		n++;
	} while (*p == ',');			/* while more parms */
	*p = '\0';
	return n;
}


/*
 * char *
 * bs_lexnum(const char *p, int *parm)
 *	Extract number from string (works with octal, decimal, or hex)
 *
 * Calling/Exit State:
 *	     p = pointer to input string
 *	  parm = pointer to value storage
 *
 *	returns 0 = error
 *		else pointer to next non-numeric character in input string
 *
 *	octal constants are preceded by a '0'
 *	hex constants are preceded by a '0x'
 *	all others are assumed decimal
 */
char *
bs_lexnum(const char *p, int *parm)
{
	char *q;
	int neg = 0;

	if (*p == '-') {
		neg++;
		p++;
	}
	if (*p == '0') {			/* hex or octal */
		p++;
		if (*p == 'x') {		/* hex */
			p++;
			q = bs_stratoi(p, parm, BASE16);
		} else {			/* octal */
			p--;			/* leave leading zero */
			q = bs_stratoi(p, parm, BASE8);
		}
	} else {				/* decimal */
		q = bs_stratoi(p, parm, BASE10);
	}
	if (q == (char *)0  ||  p == q)
		return (char *)0;
	if (neg) {
		*parm = -(*parm);
	}
	return q;
}


/*
 * char *
 * bs_stratoi(const char *p, int *parm, int base)
 *	Convert ASCII numbers to integer values
 *
 * Calling/Exit State:
 *	      p = pointer to input string
 *	   parm = pointer to value storage
 *	   base = number base for conversion
 *
 *	returns NULL = no value found
 *		else pointer to next non-numeric character
 */
char *
bs_stratoi(const char *p, int *parm, int base)
{
	char *q, *digits;
	static char *digit[] = {"01234567", "0123456789", "0123456789abcdef"};
	static int bases[] = {8, 10, 16};

	if (base > 2)
		return (char *)NULL;
	digits = digit[base];
	*parm = 0;
	while ((q = bs_strchr(digits, (*p>='A') ? *p|0x20 : *p)) != (char *)0) {
		*parm = (*parm) * bases[base] + (q - digits);
		p++;
	}
	return (char *)p;
}


/*
 * int
 * bs_find_bmaj(const char *dev)
 *	Find a block device with the given name
 *
 * Calling/Exit State:
 *	Returns the major number of the given device, or -1 if not found.
 */
int
bs_find_bmaj(const char *dev)
{
	int  i;
	char *cp;

	for (i = 0; i < bdevcnt; i++)
		if ((cp = bdevsw[i].d_name) && equal(dev, cp))
			return i;
	return -1;
}


/*
 * int
 * bs_find_cmaj(const char *dev)
 *	Find a character device with the given name
 *
 * Calling/Exit State:
 *	Returns the major number of the given device, or -1 if not found.
 */
int
bs_find_cmaj(const char *dev)
{
	int  i;
	char *cp;

	for (i = 0; i < cdevcnt; i++)
		if ((cp = cdevsw[i].d_name) && equal(dev, cp))
			return i;
	return -1;
}


/*
 * char *
 * bs_strchr(const char *sp, char c)
 *	Find first occurrence of c in string sp.  (Like strchr(3).)
 *
 * Calling/Exit State:
 *	Return pointer to first occurrence of c in string, or NULL if none.
 */
char *
bs_strchr(const char *sp, char c)
{
	while (*sp) {
		if (*sp == c)
			return (char *)sp;
		sp++;
	}
	return NULL;
}
