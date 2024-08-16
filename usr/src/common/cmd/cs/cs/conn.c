/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cs:cs/conn.c	1.9.2.8"
#ident	"$Header: $"

#include <unistd.h>
#include "uucp.h"
#include <netdir.h>
#include <signal.h>
#define SIGRTN  void

GLOBAL char _Protocol[40] = "";	/* working protocol string */
static char _ProtoSys[40] = "";	/* protocol string from Systems file entry */
static char _ProtoDev[40] = "";	/* protocol string from Devices file entry */
EXTERN char _ProtoCfg[];	/* protocol string from Config  file entry */

EXTERN jmp_buf Sjbuf;
EXTERN unsigned expecttime;

GLOBAL int	Modemctrl;

EXTERN void alarmtr();
static void addProto(), mergeProto(), removeProto();
static char *nextProto();
EXTERN char *findProto();
static void getProto();
static finds();
EXTERN int getto();		/* make this static when ct uses altconn() */
EXTERN int chat(), rddev(), expect(), wrstr(), wrchr();
EXTERN int processdev(), getdevline(), getsysline(), sysaccess();
EXTERN char *currsys(), *currdev();
static int finds();
extern int do_auth();

EXTERN void sendthem(), nap();
static notin(), ifdate(), classmatch();

EXTERN void ioctltr();

char *Abort_str;		/* pointer to string to abort chat script */

extern struct nd_hostserv Nd_hostserv;

extern check_device_privs();	/* checks MAC & DAC priv of device */

GLOBAL char *Myline = CNULL;	/* to force which line will be used */
GLOBAL char *Mytype = CNULL;	/* to force selection of specific device type */

/*
 * conn - place a telephone call to system and login, etc.
 *
 * return codes:
 *	FAIL - connection failed
 *	>0  - file no.  -  connect ok
 * When a failure occurs, Uerror is set.
 */

GLOBAL int
conn(system)
char *system;
{
	int nf, fn = FAIL;
	char *flds[F_MAX+1];
	EXTERN void sysreset();
	extern int  returnfd;

	CDEBUG(4, gettxt(":164", "placing a phone call to <%s>\n"), system);
	Uerror = 0;
	while ((nf = finds(system, flds, F_MAX)) > 0) {
		fn = getto(flds);
		CDEBUG(4, gettxt(":237", "getto returns file descriptor %d\n"), fn);
		if (fn < 0)
		    continue;
		if ( EQUALS(Nd_hostserv.h_serv, "cu") ||
		     EQUALS(Nd_hostserv.h_serv, "ct") ||
		     chat(nf - F_LOGIN, flds + F_LOGIN, fn,"","") == SUCCESS) {
			sysreset();
			return(fn); /* successful return */
		}

		/* login failed */
		DEBUG(6, gettxt(":238", "close caller (%d)\n"), fn);
		fd_rmlock(fn);
		close(fn);
		/* ???  fosile code  ???
		if (Dc[0] != NULLCHAR) {
			DEBUG(6, "delock line (%s)\n", Dc);
		}
		*/
	}

	/* finds or getto failed */
	sysreset();
	CDEBUG(1, gettxt(":165", "call failed: %s\n"), UERRORTEXT(Uerror));
	return(FAIL);
}

/*
 * getto - connect to remote machine
 *
 * return codes:
 *	>0  -  file number - ok
 *	FAIL  -  failed
 */

GLOBAL int
getto(flds)
char *flds[];
{
	char *dev[D_MAX+2], devbuf[BUFSIZ];
	register int status;
	register int dcf = -1;
	int reread = 0;
	int tries = 0;	/* count of call attempts - for limit purposes */
	EXTERN void devreset();

	CDEBUG(1, gettxt(":166", "Device Type %s wanted\n"), flds[F_TYPE]);
	Uerror = 0;
	while (tries < TRYCALLS) {
		if ((status=rddev(flds[F_TYPE], dev, devbuf, D_MAX)) == FAIL) {
			if (tries == 0 || ++reread >= TRYCALLS)
				break;
			devreset();
			continue;
		}
		/* check class, check (and possibly set) speed */
		if (classmatch(flds, dev) != SUCCESS) {
		    DEBUG(7, gettxt(":167", "Skipping entry in '%s'"), currdev());
		    DEBUG(7, gettxt(":168", " - class (%s) not wanted.\n"), dev[D_CLASS]);
		    continue;
		}

		/*
		 * if the user has also entered a 'device' (with a -l) then
		 * we skip the entry if there is no match... (i.e. if -l
		 * is entered we only want to dial on that line.
		 * we note that at this point the dev[D_LINE] entry has
		 * removed the /dev/ if it existed. If the user entered
		 * without a /dev/ and there is no match or they entered
		 * a /dev/ and there is no match we skip the record
		 */

		if( Call.line && (*Call.line != 0) &&
			 ((strcmp(Call.line, dev[D_LINE])) &&
			 ((strlen(Call.line) <= 5) || 
			 ((strlen(Call.line) > 5) &&
			 (strcmp(&Call.line[5],  dev[D_LINE])))))) {
		    DEBUG(7, gettxt(":167", "Skipping entry in '%s'"), currdev());
		    DEBUG(7, gettxt(":339", " - line (%s) not wanted.\n"), dev[D_LINE]);
		    continue;
		}

                DEBUG(5, gettxt(":169", "Trying device entry '%s' "), dev[D_LINE]);
		DEBUG(5, gettxt(":170", " from '%s'.\n"), currdev());
		if ((dcf = processdev(flds, dev)) >= 0)
			break;

		switch(Uerror) {
		case SS_CANT_ACCESS_DEVICE:
		case SS_DEVICE_FAILED:
		case SS_LOCKED_DEVICE:
			break;
		default:
			tries++;
			break;
		}
	}

	devreset();	/* reset devices file(s) */
	if (status == FAIL && !Uerror) {
	    CDEBUG(1, gettxt(":171", "Requested Device Type Not Found\n%s"), "");
	    Uerror = SS_NO_DEVICE;
	}
	return(dcf);
}

/*
 * classmatch - process 'Any' in Devices and Systems and
 * 	determine the correct speed, or match for ==
 */

static int
classmatch(flds, dev)
char *flds[], *dev[];
{
	/* check class, check (and possibly set) speed */
	if (EQUALS(flds[F_CLASS], "Any")
	   && EQUALS(dev[D_CLASS], "Any")) {
		dev[D_CLASS] = DEFAULT_BAUDRATE;
		return(SUCCESS);
	} else if (EQUALS(dev[D_CLASS], "Any")) {
		dev[D_CLASS] = flds[F_CLASS];
		return(SUCCESS);
	} else if (EQUALS(flds[F_CLASS], "Any") ||
	EQUALS(flds[F_CLASS], dev[D_CLASS]))
		return(SUCCESS);
	else
		return(FAIL);
}


/*
 *	rddev - find and unpack a line from device file for this caller type 
 *	lines starting with whitespace of '#' are comments
 *
 *	return codes:
 *		>0  -  number of arguments in vector - succeeded
 *		FAIL - EOF
 */

GLOBAL int
rddev(type, dev, buf, devcount)
char *type;
char *dev[];
char *buf;
{
	char *commap, d_type[BUFSIZ];
	char dev_path[BUFSIZ];
	int na;

	while (getdevline(buf, BUFSIZ)) {
		if (buf[0] == ' ' || buf[0] == '\t'
		    ||  buf[0] == '\n' || buf[0] == '\0' || buf[0] == '#')
			continue;
		na = getargs(buf, dev, devcount);
		ASSERT(na >= D_CALLER, "BAD LINE", buf, na);

		if ( strncmp(dev[D_LINE],"/dev/",5) == 0 ) {
			/* since cu (altconn()) strips off leading */
			/* "/dev/",  do the same here.  */
			strcpy(dev[D_LINE], &(dev[D_LINE][5]) );
		}

		/* may have ",M" subfield in D_LINE */
		Modemctrl = FALSE;
		if ( (commap = strchr(dev[D_LINE], ',')) != (char *)NULL ) {
			if ( strcmp( commap, ",M") == SAME )
				Modemctrl = TRUE;
			*commap = '\0';
		}

		/*
		 * D_TYPE field may have protocol subfield, which
		 * must be pulled off before comparing to desired type.
		 */
		(void)strcpy(d_type, dev[D_TYPE]);
		if ((commap = strchr(d_type, ',')) != (char *)NULL )
			*commap = '\0';

		/* to force the requested device type to be used. */
		if ((Mytype != NULL) && (!EQUALS(Mytype, d_type)) )
		    continue;
		/* to force the requested line to be used */
		if ((Myline != NULL) && (!EQUALS(Myline, dev[D_LINE])) )
		    continue;

		bsfix(dev);	/* replace \X fields */

		if (EQUALS(d_type, type)) {
			/* check MAC/DAC privs of device if not CS */
			if ( !EQUALS("CS", dev[D_CALLER]) ) {
				sprintf(dev_path,"%s%s", 
				       (dev[D_LINE][0] != '/')? 
				       "/dev/": "",  dev[D_LINE]);
				if (check_device_privs(dev_path) != 0){
					continue;
				}
			}
			getProto( _ProtoDev, dev[D_TYPE] );
			return(na);
		}
	}
	return(FAIL);
}


/*
 * finds	- set system attribute vector
 *
 * input:
 *	fsys - open Systems file descriptor
 *	sysnam - system name to find
 * output:
 *	flds - attibute vector from Systems file
 *	fldcount - number of fields in flds
 * return codes:
 *	>0  -  number of arguments in vector - succeeded
 *	FAIL - failed
 * Uerror set:
 *	0 - found a line in Systems file
 *	SS_BADSYSTEM - no line found in Systems file
 *	SS_TIME_WRONG - wrong time to call
 */

static int
finds(sysnam, flds, fldcount)
char *sysnam, *flds[];
{
	static char info[BUFSIZ];
	int na;

	/* format of fields
	 *	0 name;
	 *	1 time
	 *	2 acu/hardwired
	 *	3 speed
	 *	etc
	 */
	if (sysnam == 0 || *sysnam == 0 ) {
		Uerror = SS_BADSYSTEM;
		return(FAIL);
	}

	while (getsysline(info, sizeof(info))) {
		na = getargs(info, flds, fldcount);
		bsfix(flds);	/* replace \X fields */
		if ( !EQUALSN(sysnam, flds[F_NAME], MAXBASENAME))
			continue;
		/* check if requested Mytype device type */
		if ((Mytype != CNULL)
		    && (!EQUALSN(flds[F_TYPE], Mytype, strlen(Mytype)))) {
		    DEBUG(7, gettxt(":167", "Skipping entry in '%s'"), currsys());
		    DEBUG(7, gettxt(":172", " - type (%s) not wanted.\n"), flds[F_TYPE]);
		    continue;
		} else {
		    DEBUG(5, gettxt(":173", "Trying entry from '%s'"), currsys());
		    DEBUG(5, gettxt(":174", " - device type %s.\n"), flds[F_TYPE]);
		}
		/* OK if not uucico (ie. ct or cu) or the time is right */
		if (!EQUALS(Progname, "uucico") || ifdate(flds[F_TIME])) {
			/*  found a good entry  */
			getProto(_ProtoSys, flds[F_TYPE]);
			Uerror = 0;
			return(na);	/* FOUND OK LINE */
		}
		CDEBUG(1, gettxt(":175", "Wrong Time To Call: %s\n"), flds[F_TIME]);
		Uerror = SS_TIME_WRONG;
	}
	if (!Uerror)
		Uerror = SS_BADSYSTEM;
	return(FAIL);
}

/*
 * getProto - get the protocol letters from the input string.
 * input:
 *	str - string from Systems/Devices/Config file,
 *		a ',' delimits the protocol string
 *		e.g. ACU,g or DK,d
 * output:
 *	str - the , (if present) will be replaced with NULLCHAR
 *
 * return:  none
 */

static void
getProto(save, str)
char *save;
char *str;
{
	register char *p;

	*save = NULLCHAR;
	if ( (p=strchr(str, ',')) != NULL) {
		*p = NULLCHAR;
		(void) strcpy(save, p+1);
		DEBUG(7, "Protocol = %s\n", save);
                /* copy protocol into Call structure to copy back over CS pipe */
	        ((CALL_EXT	*)Call.device)->protocol=strdup(save);
	}
	return;
}

/*
 * check for a specified protocol selection string
 * return:
 *	protocol string pointer
 *	NULL if none specified for LOGNAME
 */
GLOBAL char *
protoString(valid)
char *valid;
{
	char *save;
	
	save =strdup(valid);
	_Protocol[0] = '\0';

	if ( _ProtoSys[0] != '\0' )
	    addProto(_ProtoSys, valid);
	if ( _ProtoDev[0] != '\0' )
	    addProto(_ProtoDev, valid);
	if ( _ProtoCfg[0] != '\0' )
	    addProto(_ProtoCfg, valid);

	if ( _Protocol[0] == '\0' ) {
	    (void) strcpy(valid, save);
	    (void) strcpy(_Protocol, save);
	}

	return(_Protocol[0] == NULLCHAR ? NULL : _Protocol);
}

/*
 * addProto
 *
 * Verify that the desired protocols from the Systems and Devices file
 * have been compiled into this application.
 *
 * 	desired -	list of desired protocols
 *	valid -		list of protocols that are compiled in.
 */

static void
addProto (desired, valid)
char *desired;
char *valid;
{
	register char * protoPtr;
	register char *	wantPtr;

	if ( *desired == '\0' )
	    return;

	if ( *(protoPtr = _Protocol) != NULLCHAR ) {
	    while ( *(protoPtr = nextProto(protoPtr)) != NULLCHAR ) {
		if ( *(wantPtr = findProto(desired, *protoPtr)) == NULLCHAR ) {
		    removeProto(valid, *protoPtr);	
		    removeProto(protoPtr, *protoPtr);	
		} else {
		    mergeProto(protoPtr, wantPtr);
		    protoPtr++;
		}
	    }
	} else {
	    wantPtr = desired;
	    while ( *(wantPtr = nextProto(wantPtr)) != NULLCHAR ) {
		if ( *(findProto(valid, *wantPtr)) != NULLCHAR ) {
		    mergeProto(protoPtr, wantPtr);
		}
		wantPtr++;
	    }
	}
	if ( *(protoPtr = _Protocol) != NULLCHAR ) {
	    while ( *(protoPtr = nextProto(protoPtr)) != NULLCHAR )
		*(valid++) = *(protoPtr++);
	    *valid = NULLCHAR;
	}
	return;
}

/*
 * mergeProto
 *
 * input
 * 	char *tostring, *fromstring;
 */
static void
mergeProto(tostring, fromstring)
char *tostring, *fromstring;
{
	char buffer[BUFSIZ];
	int length;

	while ( *(tostring = nextProto(tostring)) != NULLCHAR ) {
	    if ( *tostring == *fromstring )
		break;
	    else
		tostring++;
	}
	
	if ( *tostring == NULLCHAR ) {
	    length = nextProto(fromstring + 1) - fromstring;
	    (void) strncpy(tostring, fromstring, length);
	    *(tostring + length) = NULLCHAR;
	} else {
	    tostring++;
	    fromstring++;
	    if ( (*tostring !=  '(') && (*fromstring == '(') ) {
		(void) strcpy(buffer, tostring);
		length = nextProto(fromstring) - fromstring;
	        (void) strncpy(tostring, fromstring, length);
		(void) strcpy(tostring+length, buffer);
	    }
	}
	return;
}

/*
 * removeProto
 *
 * char *old
 * char letter
 *
 * return
 *	none
 */
static void
removeProto(string, letter)
char *string, letter;
{
	while ( *(string = nextProto(string)) != NULLCHAR ) {
	    if ( *string == letter )
		(void) strcpy(string, nextProto(string+1));
	    else
		string++;
	}
}

/* 
 * nextProto
 *	char *string;
 * return
 *	char * to next non-parameter letter
 */
static char *
nextProto(string)
char *string;
{
	if ( *string == '(' )
	    while ( *string != NULLCHAR )
		if ( *(string++) == ')' )
		    break;
	return(string);
}

/* 
 * findProto
 *	char *desired,
 *	char protoPtr;
 * return
 *	char *pointer to found or string terminating NULLCHAR
 */
GLOBAL char *
findProto(string, letter)
char *string;
char letter;
{
	while ( *(string = nextProto(string)) != NULLCHAR )
	    if ( *string == letter )
		break;
	    else
		string++;
	return(string);
}

/*
 * chat -	do conversation
 * input:
 *	nf - number of fields in flds array
 *	flds - fields from Systems file
 *	fn - write file number
 *	phstr1 - phone number to replace \D
 *	phstr2 - phone number to replace \T
 *
 *	return codes:  0  |  FAIL
 */

GLOBAL int
chat(nf, flds, fn, phstr1, phstr2)
char *flds[], *phstr1, *phstr2;
int nf, fn;
{
	char		*want;
	char		*altern;
	int		k, ok;

	Abort_str = NULL;

	for (k = 0; k < nf; k += 2) {
		want = flds[k];

		while (want != NULL) {
			if ((altern = index(want, '-')) != NULL)
				*altern++ = NULLCHAR;

			if (EQUALS(want, "ABORT") || EQUALS(want, "INVOKE"))
				break;

			if (expect(want, fn) == SUCCESS)
				break;

			if (altern == NULL) {
				Uerror = SS_LOGIN_FAILED;
				/* logent(UERRORTEXT, "FAILED"); */
				return(FAIL);
			}

			if ((want = index(altern, '-')) != NULL)
				*want++ = NULLCHAR;

			sendthem(altern, fn, phstr1, phstr2);
		}

		sleep(2);

		if (EQUALS(want, "ABORT")) {
			Abort_str = flds[k+1];
		} else if (EQUALS(want, "INVOKE")) {
			/* since flds[k+1] should already contain the "-r"
			   the 3rd parameter (role) is blank		*/
			if (do_auth(fn, flds[k+1], "") != 0) {
				Uerror = SS_INVOKE_FAILED;
				/* logent(UERRORTEXT, "FAILED"); */
				return(FAIL);
			}
		} else if (flds[k+1])
			sendthem(flds[k+1], fn, phstr1, phstr2);
	}

	return(0);
}

#define MR 300 

/*
 *	expect(str, fn)	look for expected string
 *	char *str;
 *
 *	return codes:
 *		0  -  found
 *		FAIL  -  lost line or too many characters read
 *		some character  -  timed out
 */

GLOBAL int
expect(str, fn)
char *str;
int fn;
{
	static char rdvec[MR];
	char *rp = rdvec;
	register int kr, c;
	char nextch;
	char *timeptr;
	int timeout;

	*rp = 0;

	CDEBUG(4, "expect: (%s", "");
	for (c=0; (kr=str[c]) != 0 ; c++)
		if (kr < 040) {
			CDEBUG(4, "^%c", kr | 0100);
		} else
			CDEBUG(4, "%c", kr);
	CDEBUG(4, ")\n%s", "");

	if (!*str) {
		CDEBUG(4, "got it\n%s", "");
		return(0);
	}

	if ((timeptr = strrchr(str, '~')) != NULL) {
		*timeptr++ = NULLCHAR;
		timeout = atoi(timeptr);
		if (timeout <= 0)
			timeout = expecttime;
	} else
		timeout = expecttime;

	if (setjmp(Sjbuf))
		return(FAIL);

	(void) signal(SIGALRM, alarmtr);
	alarm((unsigned)timeout);
	while (notin(str, rdvec)) {
		errno = 0;
		if (Abort_str && !notin(Abort_str, rdvec)) {
			alarm(0);
			DEBUG(1, "Call aborted on '%s'\n", Abort_str);
			return(FAIL);
		}
		kr = (*Read)(fn, &nextch, 1);
		if (kr <= 0) {
			alarm(0);
			CDEBUG(4, "lost line errno - %d\n", errno);
			logent("LOGIN", "LOST LINE");
			return(FAIL);
		}
		c = nextch & 0177;
		CDEBUG(4, "%s", c < 040 ? "^" : "");
		CDEBUG(4, "%c", c < 040 ? c | 0100 : c);
		if ((*rp = nextch & 0177) != NULLCHAR)
			rp++;
		if (rp >= rdvec + MR) {
			register char *p;
			for (p = rdvec+MR/2; p < rp; p++)
				*(p-MR/2) = *p;
			rp -= MR/2;
		}
		*rp = NULLCHAR;
	}
	alarm(0);
	CDEBUG(4, "got it\n%s", "");
	return(0);
}


/*
 *	alarmtr()  -  catch alarm routine for "expect".
 */
/*ARGSUSED*/
GLOBAL void
alarmtr(sig)
int sig;
{
	CDEBUG(6, gettxt(":181", "timed out\n%s"), "");
	longjmp(Sjbuf, 1);
}




/*
 *	sendthem(str, fn, phstr1, phstr2)	send line of chat sequence
 *	char *str, *phstr;
 *
 *	return codes:  none
 */

#define FLUSH() {\
	if ((bptr - buf) > 0)\
		if (wrstr(fn, buf, bptr - buf, echocheck) != SUCCESS)\
			goto err;\
	bptr = buf;\
}

GLOBAL void
sendthem(str, fn, phstr1, phstr2)
char *str, *phstr1, *phstr2;
int fn;
{
	int sendcr = 1, echocheck = 0;
	char	*sptr;
	char	*bptr;
	char	buf[BUFSIZ];
	struct termio	ttybuf;
	SIGRTN	(*SigWas)();    /* Caller's alarm handler */


	/* should be EQUALS, but previous versions had BREAK n for integer n */
	if (PREFIX("BREAK", str)) {
		/* send break */
		CDEBUG(5, gettxt(":182", "BREAK character\n"), "");
		(*genbrk)(fn);
		return;
	}

	if (EQUALS(str, "EOT")) {
		CDEBUG(5, gettxt(":183", "EOT character\n"), "");
		(void) (*Write)(fn, EOTMSG, strlen(EOTMSG));
		return;
	}

	if (EQUALS(str, "\"\"")) {
		CDEBUG(5, "\"\"\n", "");
		str += 2;
	}

	bptr = buf;
	CDEBUG(5, gettxt(":184", "sending: \n"), "");
	for (sptr = str; *sptr; sptr++) {
		if (*sptr == '\\') {
			switch(*++sptr) {

			/* adjust switches */
			case 'c':	/* no CR after string */
				FLUSH();
				if (sptr[1] == NULLCHAR) {
				    CDEBUG(5, gettxt(":185", "<no CR character>\n"), "");
				    sendcr = 0;
				} else
				    CDEBUG(5, gettxt(":186", "<request for no CR character is ignored>\n"), "");
				continue;
			}
\
			/* stash in buf and continue */
			switch(*sptr) {
			case 'D':	/* raw phnum */
				strcpy(bptr, phstr1);
				bptr += strlen(bptr);
				continue;
			case 'T':	/* translated phnum */
				strcpy(bptr, phstr2);
				bptr += strlen(bptr);
				continue;
			case 'N':	/* null */
				*bptr++ = 0;
				continue;
			case 's':	/* space */
				*bptr++ = ' ';
				continue;
			case '\\':	/* backslash escapes itself */
				*bptr++ = *sptr;
				continue;
			default:	/* send the backslash */
				*bptr++ = '\\';
				*bptr++ = *sptr;	
				continue;

			/* flush buf, perform action, and continue */
			case 'E':	/* echo check on */
				FLUSH();
				CDEBUG(5, gettxt(":187", "<echo check is on>\n"), "");
				echocheck = 1;
				continue;
			case 'e':	/* echo check off */
				FLUSH();
				CDEBUG(5, gettxt(":188", "<echo check is off>\n"), "");
				echocheck = 0;
				continue;
			case 'd':	/* sleep briefly */
				FLUSH();
				CDEBUG(5, gettxt(":189", "<delay 2 seconds>\n"), "");
				sleep(2);
				continue;
			case 'p':	/* pause momentarily */
				FLUSH();
				CDEBUG(5, gettxt(":190", "<pause approximately 0.25 seconds>\n"), "");
				nap(HZ/4);	/* approximately 1/4 second */
				continue;
			case 'K':	/* inline break */
				FLUSH();
				CDEBUG(5, gettxt(":191", "<inline BREAK character>\n"), "");
				(*genbrk)(fn);
				continue;
			case 'M':	/* modem control - set CLOCAL */
			case 'm':	/* no modem control - clear CLOCAL */
				FLUSH();
				if ( (*Ioctl)(fn, TCGETA, &ttybuf) != 0  ) {
				    CDEBUG(5, gettxt(":192", "<CLOCAL option ignored; ioctl(TCGETA) failed, errno=%d>\n"), errno);
				} else {
				    if (*sptr == 'M') {
					CDEBUG(5, gettxt(":193", "<setting CLOCAL option>\n"), "");
					ttybuf.c_cflag |= CLOCAL;
				    } else {
					CDEBUG(5, gettxt(":194", "<clearing CLOCAL option>\n"), "");
					ttybuf.c_cflag &= ~CLOCAL;
				    }
                		    SigWas = signal(SIGALRM, ioctltr);
                		    alarm(10);
				    if ( (*Ioctl)(fn, TCSETAW, &ttybuf) != 0 )
					CDEBUG(5, gettxt(":195", "<CLOCAL option ignored; ioctl(TCSETAW) failed, errno=%d>\n"), errno);
                		    signal(SIGALRM, SigWas);
				    alarm(0);
				}
				continue;
			}
		} else
			*bptr++ = *sptr;
	}
	if (sendcr)
		*bptr++ = '\r';
	if ( (bptr - buf) > 0 )
		(void) wrstr(fn, buf, bptr - buf, echocheck);

err:
	/*
	 * Need to figure out a better message.  Coming here does
	 * not automatically mean that it's en error.
	 */
	/*CDEBUG(5, gettxt(":196", "error in sending: errno=%d\n"), errno);*/
	CDEBUG(5, "\n", "");
	return;
}

#undef FLUSH

GLOBAL int
wrstr(fn, buf, len, echocheck)
char *buf;
{
	int 	i;
	char dbuf[BUFSIZ], *dbptr = dbuf;

	buf[len] = 0;

	if (echocheck)
		return(wrchr(fn, buf, len));

	if (Debug >= 5) {
		if (sysaccess(ACCESS_SYSTEMS) == 0) { /* Systems file access ok */
			for (i = 0; i < len; i++) {
				*dbptr = buf[i];
				if (iswcntrl((unsigned char)*dbptr)) {
				    sprintf(dbptr, "\\%0.3o", buf[i]);
				    dbptr += 4;
				} else {
				    *dbptr = buf[i];
				    dbptr++;
				}
			}
			*dbptr = 0;
		} else {
			strcpy(dbuf, gettxt(":197", "<Password not printed>"));
		}
		CDEBUG(5, "%s\n", dbuf);
	}

	if ((*Write)(fn, buf, len) != len)
		return(FAIL);
	return(SUCCESS);
}

GLOBAL int
wrchr(fn, buf, len)
register int fn;
register char *buf;
{
	int 	i, saccess;
	char	cin, cout;

	saccess = (sysaccess(ACCESS_SYSTEMS) == 0); /* protect Systems file */
	if (setjmp(Sjbuf))
		return(FAIL);
	(void) signal(SIGALRM, alarmtr);

	for (i = 0; i < len; i++) {
		cout = buf[i];
		if (saccess) {
			if (iswcntrl(cout))
			CDEBUG(5, "\\%0.3o", cout);
		} else {
			CDEBUG(5, "?", "");
		}
		if (((*Write)(fn, &cout, 1)) != 1)
			return(FAIL);
		do {
			(void) alarm(expecttime);
			if ((*Read)(fn, &cin, 1) != 1)
				return(FAIL);
			(void) alarm(0);
			/* can't do this anymore; need 8 bit clear for i18n */
			/* cin &= 0177; */
			if (saccess) {
				if (iswcntrl((unsigned char)cin))
				CDEBUG(5, "\\%0.3o", cin);
			} else {
				CDEBUG(5, "?", "");
			}
		} while (cout != (cin /* & 0177 */));
	}
	return(SUCCESS);
}


/*
 *	notin(sh, lg)	check for occurrence of substring "sh"
 *	char *sh, *lg;
 *
 *	return codes:
 *		0  -  found the string
 *		1  -  not in the string
 */

static int
notin(sh, lg)
char *sh, *lg;
{
	while (*lg != NULLCHAR) {
		if (PREFIX(sh, lg))
			return(0);
		else
			lg++;
	}
	return(1);
}


/*
 *	ifdate(s)
 *	char *s;
 *
 *	ifdate  -  this routine will check a string (s)
 *	like "MoTu0800-1730" to see if the present
 *	time is within the given limits.
 *	SIDE EFFECT - Retrytime is set to number following ";"
 *
 *	String alternatives:
 *		Wk - Mo thru Fr
 *		zero or one time means all day
 *		Any - any day
 *
 *	return codes:
 *		0  -  not within limits
 *		1  -  within limits
 */

static int
ifdate(s)
char *s;
{
	static char *days[] = {
		"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa", 0
	};
	time_t	clock;
	int	t__now;
	char	*p;
	struct tm	*tp;

	time(&clock);
	tp = localtime(&clock);
	t__now = tp->tm_hour * 100 + tp->tm_min;	/* "navy" time */

	/*
	 *	pick up retry time for failures
	 *	global variable Retrytime is set here
	 */
	if ((p = rindex(s, ';')) != NULL)
	    if (isdigit(p[1])) {
		if (sscanf(p+1, "%ld", &Retrytime) < 1)
			Retrytime = 5;	/* 5 minutes is error default */
		Retrytime  *= 60;
		*p = NULLCHAR;
	    }

	while (*s) {
		int	i, dayok;

		for (dayok = 0; (!dayok) && isalpha(*s); s++) {
			if (PREFIX("Any", s))
				dayok = 1;
			else if (PREFIX("Wk", s)) {
				if (tp->tm_wday >= 1 && tp->tm_wday <= 5)
					dayok = 1;
			} else
				for (i = 0; days[i]; i++)
					if (PREFIX(days[i], s))
						if (tp->tm_wday == i)
							dayok = 1;
		}

		if (dayok) {
			int	t__low, t__high;

			while (isalpha(*s))	/* flush remaining day stuff */
				s++;

			if ((sscanf(s, "%d-%d", &t__low, &t__high) < 2)
			 || (t__low == t__high))
				return(1);

			/* 0000 crossover? */
			if (t__low < t__high) {
				if (t__low <= t__now && t__now <= t__high)
					return(1);
			} else if (t__low <= t__now || t__now <= t__high)
				return(1);

			/* aim at next time slot */
			if ((s = index(s, ',')) == NULL)
				break;
		}
		if (*s)
		    s++;
	}
	return(0);
}

/*
 *	char *
 *	fdig(cp)	find first digit in string
 *
 *	return - pointer to first digit in string or end of string
 */

GLOBAL char *
fdig(cp)
char *cp;
{
	char *c;

	for (c = cp; *c; c++)
		if (*c >= '0' && *c <= '9')
			break;
	return(c);
}


#ifdef FASTTIMER
/*	Sleep in increments of 60ths of second.	*/
GLOBAL void
nap (time)
register int time;
{
	static int fd;

	if (fd == 0)
		fd = open (FASTTIMER, 0);

	(void) (*Read)(fd, 0, time);
	return;
}

#endif /* FASTTIMER */

#ifdef NONAP

/*	nap(n) where n is ticks
 *
 *	loop using n/HZ part of a second
 *	if n represents more than 1 second, then
 *	use sleep(time) where time is the equivalent
 *	seconds rounded off to full seconds
 *	NOTE - this is a rough approximation and chews up
 *	processor resource!
 */

GLOBAL void
nap(n)
unsigned n;
{
	struct tms	tbuf;
	long endtime;
	int i;

	if (n > HZ) {
		/* > second, use sleep, rounding time */
		sleep( (int) (((n)+HZ/2)/HZ) );
		return;
	}

	/* use timing loop for < 1 second */
	endtime = times(&tbuf) + 3*n/4;	/* use 3/4 because of scheduler! */
	while (times(&tbuf) < endtime) {
	    for (i=0; i<1000; i++, (void) (i*i))
		;
	}
	return;
}

#endif /* NONAP */

/*
 * altconn - place a telephone call to system 
 * from cu when telephone number or direct line used
 *
 * return codes:
 *	FAIL - connection failed
 *	>0  - file no.  -  connect ok
 * When a failure occurs, Uerror is set.
 */

/* it looks like nobody calls this function */

GLOBAL int
altconn(call)
struct call *call;
{
	int fn = FAIL;
	char *alt[7];
	EXTERN char *Myline;

	alt[F_NAME] = "dummy";	/* to replace the Systems file fields  */
	alt[F_TIME] = "Any";	/* needed for getto(); [F_TYPE] and    */
	alt[F_TYPE] = "";	/* [F_PHONE] assignment below          */
	alt[F_CLASS] = call->speed;
	alt[F_PHONE] = "";
	alt[F_LOGIN] = "";
	alt[6] = NULL;

	CDEBUG(4,"altconn called\n", "");

	/* cu -l dev ...					*/
	/* if is "/dev/device", strip off "/dev/" because must	*/
	/* exactly match entries in Devices file, which usually	*/
	/* omit the "/dev/".  if doesn't begin with "/dev/",	*/
	/* either they've omitted the "/dev/" or it's a non-	*/
	/* standard path name.  in either case, leave it as is	*/

	if(call->line != NULL ) {
		if ( strncmp(call->line, "/dev/", 5) == 0 ) {
			Myline = (call->line + 5);
		} else {
			Myline = call->line;
		}
	}

	/* cu  ... telno */
	if(call->telno != NULL) {
		alt[F_PHONE] = call->telno;
		alt[F_TYPE] = "ACU";
	} else {
	/* cu direct line */
		alt[F_TYPE] = "Direct";
	}
	if (call->type != NULL)
		alt[F_TYPE] = call->type;
	fn = getto(alt);
	CDEBUG(4, "getto ret %d\n", fn);

	return(fn);

}

