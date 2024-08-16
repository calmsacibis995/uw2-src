/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/msg.c	1.19"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: msg.c,v 1.26.4.1 1995/02/02 23:28:06 vtag Exp $"

/*
 * Copyright 1991, 1993 Unpublished Work of Novell, Inc.
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "nwmsg.h"  /* must come before limits.h */
#include <nl_types.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "memmgr_types.h"
#include <util_proto.h>

#if defined(OS_SUN4)
/*
** These should be declared in <nl_types.h> but seem to be leftout.
*/
extern char *catgets(nl_catd catd, int set_num, int msg_num, char *s);
extern nl_catd catopen(char *name, int oflag);
extern int catclose(nl_catd catd);
#endif

#include <locale.h>
#include <time.h>
#include <nwconfig.h>
#include <mt.h>

#ifdef _REENTRANT
#include "nwutil_mt.h"

MUTEX_T fileStructLock;
MUTEX_T defLangLock;
MUTEX_T defNLSPathLock;
MUTEX_T setLocaleLock;
MUTEX_T catalogArrayInitializedLock;
#endif

static int catalogArrayInitialized = 0;

#if !defined(PATH_MAX)
#define PATH_MAX 255
#endif

extern char * getenv();

void
ConvertToUpper(char *s)
{
      char *cp;

      for (cp = s; *cp; cp++)
              *cp = (char)toupper((int)*cp);
      return;
}

/*
** Domain files.  One descriptor and base filename and revision string 
** for each index.  The index into file[] is the file number as used to 
** setup the Message Domains in ** msg.h.  Currently only 1 file is defined.
*/
struct fileStruct {
	nl_catd catds;			/* catalog desriptor */
	char	*fileName;		/* Default basename (used if msgFileName to function
							** MsgBindDomain() is NULL)
							*/
	char	*revStr;		/* Default RevStr (used if msgFileName to Function
							** MsgBindDomain() is NULL)
							*/
} file[MSG_MAX_DOMAINS_FILES];

typedef struct domainStruct {
	nl_catd		catd;
	int 		set; 
	int			fileNumber;
} Domain_t;
static Domain_t curDomain = { (nl_catd)-1, -1, -1 };
static unsigned char statBuf[MSG_MAX_LEN];


static unsigned char *
getMsgBuf()
{
	unsigned char	*buf;

#ifdef _REENTRANT
	struct msg_tsd  *key_tbl;

	if(FIRST_OR_NO_THREAD) {
		buf = statBuf;
	} else {

		key_tbl = (struct msg_tsd *)_mt_get_thr_specific_storage(
						domainKey, MSG_TSD_KEYTBL_SIZE);
		if(key_tbl == NULL)
			return( NULL );

		if(key_tbl->buf == NULL)
		{
			key_tbl->buf = (unsigned char *)calloc(1, MSG_MAX_LEN);
		}
		buf = key_tbl->buf;
	}

#else /* ! _REENTRANT */

	buf = statBuf;
#endif

	return( buf );
}


static Domain_t *
getDomain()
{
	Domain_t	*domain;

#ifdef _REENTRANT
	struct msg_tsd  *key_tbl;
	Domain_t *key_domain;

	if(FIRST_OR_NO_THREAD) {
		domain = &curDomain;
	} else {
		key_tbl = (struct msg_tsd *)_mt_get_thr_specific_storage(
						domainKey, MSG_TSD_KEYTBL_SIZE);
		if(key_tbl == NULL)
			return( NULL );

		if(key_tbl->curDomain == NULL)
		{
			key_domain = (Domain_t *)calloc(1, sizeof(Domain_t));
			key_tbl->curDomain = key_domain;
			key_domain->catd = (nl_catd)-1;
			key_domain->set = -1;
			key_domain->fileNumber = -1;
		}
		domain = key_tbl->curDomain;
	}

#else /* ! _REENTRANT */

	domain = &curDomain;
#endif

	return( domain );
}

int
msgSetDefLang(void)
{
	static int setDefLangOK = FALSE;
	
	MUTEX_LOCK(&defLangLock);
	if(setDefLangOK == TRUE) {
		goto OUT;
	}

	/* If LANG is set in environment, do nothing. */
	if(getenv("LANG")) {
		goto OUT;
	}	
	putenv("LANG=C");

 OUT:
	setDefLangOK = TRUE;
	MUTEX_UNLOCK(&defLangLock);

	return SUCCESS;
}

int
msgSetDefNlsPath(void)
{
	char binDir[NWCM_MAX_STRING_SIZE];
	char *envPtr;
	struct stat statBuf;
	static char envStr[PATH_MAX+12];  /* must be static! */
	int ccode;
	static int setDefNlsPathOK = FALSE;

	MUTEX_LOCK(&defNLSPathLock);
	if(setDefNlsPathOK == TRUE)	{
		ccode = SUCCESS;
		goto OUT;
	}

	ccode = NWCMGetParam("binary_directory", NWCP_STRING, (void *)binDir);
	if(ccode != NWCM_SUCCESS) {
		goto OUT;
	}

	/*
	**	If NLSPATH is set in environment, do nothing.
	**	Don't you dare move this code before the NWCMGetParam, if you
	**	do, nwcm will not get initialized.  It must be initialized
	*/

	if(getenv("NLSPATH")) {
		setDefNlsPathOK = TRUE;
		ccode = SUCCESS;
		goto OUT;
	}	

	strcat(binDir, "/nls/English");
	if(stat(binDir, &statBuf) == 0) {
		/* Directory exists. Create a default NLSPATH that points to
		 * <binary_directory>/nls/English/%N.cat.
		 */
		strcat(binDir, "/%N.cat");
	}
	else {
		/* Directory doesn't exist, Create a default X/Open NLSPATH */
		strcpy(binDir, "/usr/lib/locale/%L/LC_MESSAGES/%N.cat");
	}

	/* Prepend the default NLSPATH to the current value of NLSPATH. */
	envPtr = getenv("NLSPATH");

	if(envPtr == (char *)NULL)
		sprintf(envStr, "NLSPATH=%s", binDir);
	else
		sprintf(envStr, "NLSPATH=%s:%s", envPtr, binDir);

	putenv(envStr);

	setDefNlsPathOK = TRUE;
	ccode = SUCCESS;

 OUT:
	MUTEX_UNLOCK(&defNLSPathLock);
	return ccode;
}


static int setlocaleOK = FALSE;

int
MsgSetlocale(void)
{
	int ccode = SUCCESS;

	MUTEX_LOCK( &setLocaleLock );
	if (setlocaleOK == FALSE) {
		if(msgSetDefLang() != SUCCESS) {
#ifdef DEBUG
			printf("msgSetDefLang failed\n");
#endif
			ccode = FAILURE;
			goto OUT;
		}
		if((ccode = msgSetDefNlsPath()) != SUCCESS) {
#ifdef DEBUG
			printf("msgSetDefNlsPath failed\n");
#endif
			goto OUT;
		}
		setlocaleOK = TRUE;
	}

	if (setlocale(LC_ALL,"") == (char *)NULL) {
#ifdef DEBUG
		printf("setlocale failed\n");
#endif
		ccode = FAILURE;
		goto OUT;
	}

 OUT:
	MUTEX_UNLOCK( &setLocaleLock );
	return ccode;
}

int
MsgGetDomain(void)
{
	Domain_t	*myDomain;

/*
**	Get domain table  -MJW
*/
	if((myDomain = getDomain()) == NULL)
		return( -1 );

	if (myDomain->catd == (nl_catd)-1) {
		return -1;		/* not set yet */
	}
	return (myDomain->fileNumber << 8) + (myDomain->set & 0x00ff);
}

int
MsgChangeDomain(int domain)
{
	int fileNumber;
	int set;
	nl_catd catd;
	Domain_t	*myDomain;

	if (domain == -1) {
		return SUCCESS;					/* Ingnor and return */
	}

	fileNumber = domain >> 8;			/* File index			*/

#if defined(DEBUG)
	if (fileNumber >= MSG_MAX_DOMAINS_FILES || fileNumber < 0) {
		fprintf(stderr, "MsgChangeDomain: Bad domain number (%d)\n", domain);
		return FAILURE;
	}
#endif

	set = domain & 0x00ff;    		/* Set number			*/

	MUTEX_LOCK( &fileStructLock );
	catd = file[fileNumber].catds; 	/* Catalog discriptor	*/
	if (catd == (nl_catd)-1) {
		MUTEX_UNLOCK( &fileStructLock );
		return FAILURE;
	}
	/*
	** They are vaild so set them
	*/

/*
**	Get domain table  -MJW
*/
	if((myDomain = getDomain()) == NULL) {
		MUTEX_UNLOCK(&fileStructLock);
		return( FAILURE );
	}

	myDomain->catd = catd;
	myDomain->fileNumber = fileNumber;
	myDomain->set = set;

	MUTEX_UNLOCK( &fileStructLock );

	return SUCCESS;
}

int 
MsgBindDomain(int domain, char *msgFileName, char *msgRevStr)
{
	return(MsgBindDomainFunc(domain, msgFileName, msgRevStr, TRUE));
}

int 
MsgBindDomainFunc(
	int 	domain,
	char	*msgFileName,
	char	*msgRevStr,
	int 	setDefault )
{
    char *sptr;
    int fileNumber, set, ccode, i;
    nl_catd catd;
	Domain_t *myDomain;
#ifdef DEBUG
	int catlen, revlen;
    char *catptr, *revptr;
#endif

    if(msgFileName == NULL || msgRevStr == NULL) {
#ifdef DEBUG
		fprintf(stderr, "MsgBindDomain: msgFileName or msgRevStr is NULL.\n");
#endif
		ccode = FAILURE;
		goto MSG_BIND_DOMAIN_EXIT_3;
    }

    /* Get file and set numbers. Check for vaild values */
    fileNumber = domain >> 8;
    set = domain & 0x00ff;

#ifdef DEBUG
    if (fileNumber >= MSG_MAX_DOMAINS_FILES || fileNumber < 0) {
		fprintf(stderr, "MsgBindDomain: Bad domain number (%d)\n", domain);
		ccode = FAILURE;
		goto MSG_BIND_DOMAIN_EXIT_3;
    }
#endif

	MUTEX_LOCK(&catalogArrayInitializedLock);
	if(catalogArrayInitialized == 0) {
		for(i=0; i<MSG_MAX_DOMAINS_FILES; i++) {
			file[i].catds = (nl_catd) -1;
			file[i].fileName = NULL;
			file[i].revStr = NULL;
		}
	}
	catalogArrayInitialized = 1;
	MUTEX_UNLOCK(&catalogArrayInitializedLock);

	MUTEX_LOCK(&fileStructLock);

	/* Get the catalog discriptor */
    if((catd = file[fileNumber].catds) != (nl_catd)-1) {
		/* File is already open */
		goto MSG_BIND_DOMAIN_EXIT_1;
    }

    /* Set the locale ONLY if it has not been done */
    if(setlocaleOK == FALSE) {
		if((ccode = MsgSetlocale()) != SUCCESS) {
#ifdef DEBUG
			printf("Setlocale LANG=C\n");
#endif
			putenv("LANG=C");
			if((ccode = MsgSetlocale()) != SUCCESS) {
#ifdef DEBUG
				printf("MsgBindDomain: MsgSetlocale() failed.\n");
#endif
				goto MSG_BIND_DOMAIN_EXIT_2;
			}
		}
    }

    /* Open the catalog file */
    if((catd = catopen(msgFileName, 0)) == (nl_catd)-1) {
#ifdef DEBUG
		printf("Set LANG=C\n");
#endif
		putenv("LANG=C");
		if((ccode = MsgSetlocale()) != SUCCESS) {
#ifdef DEBUG
			printf("MsgBindDomain: MsgSetlocale() failed.\n");
#endif
			goto MSG_BIND_DOMAIN_EXIT_2;
		}
		if((catd = catopen(msgFileName, 0)) == (nl_catd)-1) {
#ifdef DEBUG
			fprintf(stderr, "MsgBindDomain: Cannot open message catalog %s.\n", msgFileName);
#endif
			ccode = FAILURE;
			goto MSG_BIND_DOMAIN_EXIT_2;
		}
    }
#ifdef DEBUG
	else {
		printf("catopen OK\n");
	}
#endif
	/* Get the rev string. If it's Null, something is wrong with the
	 * catalog file. On AIX, catopen returns success even if it didn't
	 * find the catalog file.
	 */
	if((sptr = catgets(catd, MSG_REV_SET, MSG_REV_NUMBER, "")) ==  NULL) {
#ifdef DEBUG
		fprintf(stderr,"MsgBindDomain: Message catalog revision string is NULL.\n");
#endif
		catclose(catd);
		ccode = FAILURE;
		goto MSG_BIND_DOMAIN_EXIT_2;
	}

#ifdef DEBUG
    /* Ensure that the catalog file is up to date */
    if(msgRevStr != (char *)NULL) {
		/* Position a pointer to the strings at the beginning of the 
		 * path name.
		 */
		if((catptr = strchr(sptr, '/')) == NULL) {
			catptr = sptr;
		}
		if((revptr = strchr(msgRevStr, '/')) == NULL) {
			revptr = msgRevStr;
		}

		/* Figure the correct number of characters to compare.
		 * One of the strings may have a trailing '$' that should
		 * be ignored.
		 */
		catlen = strlen(catptr);
		if(strrchr(catptr, '$') != NULL) {
			catlen--;
		}

		revlen = strlen(revptr);
		if(strrchr(revptr, '$') != NULL) {
			revlen--;
		}

		/*
		 * Now, pick the shortest of the two lengths and store
		 * it in catlen.
		 */
		if(revlen < catlen) {
			catlen = revlen;
		}

		/*
		 * One of the strings could have been NULL. This will make
		 * sure we don't seg fault.
		 */
		if(catlen < 0) {
			catlen = 0;
		}

		if(strncmp(catptr, revptr, catlen)) {
			fprintf(stderr, "MsgBindDomain warning: Message catalog %s revision \"%s\"\n"
					"does not match internal revision \"%s\". Continuing...\n",
					msgFileName, catptr, revptr);
		}
    }
#endif

    /* Locale and revision string are O.K. */
    file[fileNumber].catds = catd;

 MSG_BIND_DOMAIN_EXIT_1:
	/* Get the domain table */
	if((myDomain = getDomain()) == NULL) {
		ccode = FAILURE;
		goto MSG_BIND_DOMAIN_EXIT_2;
	}

	if(setDefault) {
		if(myDomain->catd == (nl_catd) -1) {
			/* Set default domain only on first invocation of this function. */
			/* Note: myDomain points to THREAD specific storage. */
			myDomain->catd = catd;
			myDomain->fileNumber = fileNumber;
			myDomain->set = set;
		}
	}

	ccode = SUCCESS;

 MSG_BIND_DOMAIN_EXIT_2:
	MUTEX_UNLOCK(&fileStructLock);

 MSG_BIND_DOMAIN_EXIT_3:
    return ccode;
}

/* Scan for the [+|-]HH:MM:SS string */
/* return time it finds in *tz.*/
/* returns ch pointing to next character */
/* This code was patterned after native OSScanForTimeOffset */
/* If you have access to the strptime function, change the ifndef below */

void
MsgScanOffsetAmountStr(char **ch, long *tz)
{
	int	sign;
	long	temp;

	*tz = 0;
	sign = 1;

	/* Look for an (optinal) sign */
	if (**ch == '-')
	{
		sign = -1;
		(*ch)++;
	}
	else
	{
		if (**ch == '+')
		{
			sign = 1;
			(*ch)++;
		}
	}

#ifndef STRPTIME
	temp = 0;
	while (isdigit(**ch))
	{
		temp = temp * 10 + **ch - '0';
		(*ch)++;
	}

	/* convert hours to seconds */
	*tz	= temp * 60 * 60 * sign;
	
	/* Look for minutes */
#ifdef NATIVE
	if (**ch != DOSCountryInfo.timeSep[0])
#else
	/*
	 *	This needs to be made portable!!  I18N 
	 */
	if (**ch != ':')
#endif
	{
		return;
	}

	(*ch)++;
	temp = 0;

	while (isdigit(**ch))
	{
		temp = temp * 10 + **ch - '0';
		(*ch)++;
	}

	/* convert minutest to seconds and add to hours*/
	 *tz += (temp * 60 * sign);

	/* look for seconds */
#ifdef NATIVE
	if (**ch != DOSCountryInfo.timeSep[0])
#else
	/*
	 *	This needs to be made portable!!  I18N
	 */
	if (**ch != ':')
#endif
	{
		return;
	}

	(*ch)++;
	temp = 0;

	while (isdigit(**ch))
	{
		temp = temp * 10 + **ch - '0';
		(*ch)++;
	}

	/* add seconds to hours and minutes*/
	*tz += (temp * sign);
#else
	ch = strptime(ch, "%X", &tm);
	*tz	= tm.tm_hour * 60 * 60 + tm.tm_min * 60 + tm.tm_sec;
	*tz *= sign;
#endif
}

/* If you have access to the strptime function, change the ifndef below.
 *
 * This code was patterned after native os/src/enable.c:ExtractFormattedDate.
 *
 * Description:  Extracts a date from a string produced by FormatDate.
 * Return values:	-1 for year, month, or day indicates it was not found.
 *					buffer pointer will point past last extracted value.
 */

#define WORK_STRING_SIZE	160

#ifndef STRPTIME
char	workStr1[WORK_STRING_SIZE];
char	workStr2[WORK_STRING_SIZE];
#endif

LONG
MsgScanOffsetDateStr(BYTE **buffer, LONG *year, LONG *month, LONG *day)
{
	char	*bp, *tp;
	LONG	ccode = 0;
	BYTE	monthFlag = 0;
	int		i;
	struct tm tm;

	bp = (char *)*buffer;

	/* Show that we were not able to extract */
	*year = *month = *day = LONG_MAX;

#ifndef STRPTIME
	/* To start off lets see if they likely using a "month name" format*/
	/* or a short "month number" format.  We'll do that by just looking*/
	/* for a month name in the input.*/

	memset(workStr1, 0, WORK_STRING_SIZE);

	strncat(workStr1, bp, WORK_STRING_SIZE);
	ConvertToUpper(workStr1);

	for (i=0;i<12;i++)
	{
		tm.tm_mon = i;
		strftime(workStr2, WORK_STRING_SIZE, "%B", &tm);
		ConvertToUpper(workStr2);
		if (strstr(workStr1, workStr2))
		{
			monthFlag = i+1;
			break;
		}
		strftime(workStr2, WORK_STRING_SIZE, "%b", &tm);
		ConvertToUpper(workStr2);
		if (strstr(workStr1, workStr2))
		{
			monthFlag = i+1;
			break;
		}
	}

	/* Work with the upper cased string now*/
	bp = workStr1;

	if (monthFlag)
	{
#ifdef NATIVE
		switch (DOSCountryInfo.dateFmt)
		{
				case 0: 	/* USA 		Month DD, YYYY	*/
#endif
					while (isspace(*bp))
						bp++;
					if (bp == strstr(bp, workStr2))
					{
						bp += strlen(workStr2);
						*month = monthFlag;	/* Month is 1 to 12*/
					}
					else
					{
						ccode |= 1;	/* Month not right*/
						goto done;
					}
					tp = bp;
					*day = strtol(bp, &bp, 10);
					if (tp == bp)
					{
						ccode |= 2;	/* No day present*/
						goto done;
					}
					while (isspace(*bp))
						bp++;
					if (*bp == ',')
					{
						++bp;
					}
					tp = bp;
					*year = strtol(bp, &bp, 10);
					if (tp == bp)
					{
						ccode |= 4;	/* No year present*/
						goto done;
					}
#ifdef NATIVE
					break;

				case 1: 	/* Europe	DD Month YYYY	*/
					tp = bp;
					*day = strtol(bp, &bp, 10);
					if (tp == bp)
					{
						ccode |= 2;	/* No day present*/
						goto done;
					}
					while (isspace(*bp))
						bp++;
					if (bp == strstr(bp, workStr2))
					{
						bp += strlen(workStr2);
						*month = monthFlag;	/* Month is 1 to 12*/
					}
					else
					{
						ccode |= 1;	/* Month not right*/
						goto done;
					}
					tp = bp;
					*year = strtol(bp, &bp, 10);
					if (tp == bp)
					{
						ccode |= 4;	/* No year present*/
						goto done;
					}
					break;

				case 2:		/* Japan	YYYY Month DD	*/	
					tp = bp;
					*year = strtol(bp, &bp, 10);
					if (tp == bp)
					{
						ccode |= 4;	/* No year present*/
						goto done;
					}
					while (isspace(*bp))
						bp++;
					if (bp == strstr(bp, workStr2))
					{
						bp += strlen(workStr2);
						*month = monthFlag;	/* Month is 1 to 12*/
					}
					else
					{
						ccode |= 1;	/* Month not right*/
						goto done;
					}
					tp = bp;
					*day = strtol(bp, &bp, 10);
					if (tp == bp)
					{
						ccode |= 2;	/* No day present*/
						goto done;
					}
					break;
		
				default:
					ccode |= 1+2+4;	/* day, month, and year are bad*/
					goto done;
		}
#endif
	}
	else
	{
#ifdef NATIVE
		switch (DOSCountryInfo.dateFmt)
		{
			case 0: 	/* USA 		MM/DD/YYYY	*/
#endif
				tp = bp;
				*month = strtol(bp, &bp, 10);
				if (tp == bp)
				{
					ccode |= 1;	/* month not present*/
					goto done;
				}
					while (isspace(*bp))
						bp++;
#ifdef NATIVE
				if (*bp == DOSCountryInfo.dateSep[0])
#else
				/*
				 *	This needs to be made portable!!  I18N
				 */
				if (*bp == '/')
#endif
				{
					bp++;
				}
				tp = bp;
				*day = strtol(bp, &bp, 10);
				if (tp == bp)
				{
					ccode |= 2;	/* day not present*/
					goto done;
				}
				while (isspace(*bp))
					bp++;
#ifdef NATIVE
				if (*bp == DOSCountryInfo.dateSep[0])
#else
				/*
				 *	This needs to be made portable!!  I18N
				 */
				if (*bp == '/')
#endif
				{
					bp++;
				}
				tp = bp;
				*year = strtol(bp, &bp, 10);
				if (tp == bp)
				{
					ccode |= 4;	/* year not present*/
					goto done;
				}
#ifdef NATIVE
				break;

			case 1: 	/* Europe	DD/MM/YYYY	*/
				tp = bp;
				*day = strtol(bp, &bp, 10);
				if (tp == bp)
				{
					ccode |= 2;	/* day not present*/
					goto done;
				}
				while (isspace(*bp))
					bp++;
				if (*bp == DOSCountryInfo.dateSep[0])
				{
					bp++;
				}
				tp = bp;
				*month = strtol(bp, &bp, 10);
				if (tp == bp)
				{
					ccode |= 1;	/* month not present*/
					goto done;
				}
				while (isspace(*bp))
					bp++;
				if (*bp == DOSCountryInfo.dateSep[0])
				{
					bp++;
				}
				tp = bp;
				*year = strtol(bp, &bp, 10);
				if (tp == bp)
				{
					ccode |= 4;	/* year not present*/
					goto done;
				}
				break;

			case 2:		/* Japan	YYYY/MM/DD	*/
				tp = bp;
				*year = strtol(bp, &bp, 10);
				if (tp == bp)
				{
					ccode |= 4;	/* year not present*/
					goto done;
				}
				while (isspace(*bp))
					bp++;
				if (*bp == DOSCountryInfo.dateSep[0])
				{
					bp++;
				}
				tp = bp;
				*month = strtol(bp, &bp, 10);
				if (tp == bp)
				{
					ccode |= 1;	/* month not present*/
					goto done;
				}
				while (isspace(*bp))
					bp++;
				if (*bp == DOSCountryInfo.dateSep[0])
				{
					bp++;
				}
				tp = bp;
				*day = strtol(bp, &bp, 10);
				if (tp == bp)
				{
					ccode |= 2;	/* day not present*/
					goto done;
				}
				while (isspace(*bp))
					bp++;
				if (*bp == DOSCountryInfo.dateSep[0])
				{
					bp++;
				}
				break;

			default:
				ccode |= 1+2+4;	/* day, month, and year are bad*/
				goto done;
		}
#endif /* NATIVE */

#else /* ifndef STRPTIME */

	**buffer = strptime(*buffer, "%x", &tm);
	year = tm.tm_year;
	month = tm.tm_mon;
	day = tm.tm_mday;

#endif /* ifndef STRPTIME */
	}

	/* Now check for valid ranges on everything */
	if (*year == LONG_MAX)
	{
		ccode |= 4;
		goto done;		/* year was bad */
	}

	if (*year <= 79)	*year += 2000;	/* 0 to 79 is 2000 to 20079 */
	if (*year <= 99)	*year += 1900;	/* 80 to 99 is 1980 to 1999*/
	if (*year < 1980 || *year > 2079)
	{
		ccode |= 4;
		goto done;		/* year is out of range */
	}

	if (*month == 0 || *month > 12)
	{
		ccode |= 1;
		goto done;		/* month is bad */
	}

	switch (*month)
	{
		/* months with 31 days maximum */
		case 1:
		case 3:
		case 5:
		case 7:
		case 8:
		case 10:
		case 12:
			if (*day > 31)
			{
				ccode |= 2;	/* day is bad */
				goto done;
			}
			break;

		case 2:
			if (((*year & 0x03) && (*day > 28)) ||
				(*day > 29))
			{
				ccode |= 2;	/* day is bad */
				goto done;
			}
			break;

		case 4:
		case 6:
		case 9:
		case 11:
			if (*day > 30)
			{
				ccode |= 2;	/* day is bad*/
				goto done;
			}
			break;
	}

done:
#ifndef STRPTIME
	*buffer += (bp-workStr1);	/* Return pointer to current position in data*/
#endif
	return ccode;
}


/* If you have access to the strptime function, change the ifndef below.
 *
 * This code was patterned after native os/src/enable.c:ExtractFormattedTime.
 */

LONG
MsgScanOffsetTimeStr(BYTE **buffer, LONG *hour, LONG *minute, LONG *second)
{
	char	*bp, *tp;
	LONG	ccode = 0;
	BYTE	ampmFlag = 0;
	struct tm tm;


#ifndef STRPTIME

	memset(workStr1, 0, WORK_STRING_SIZE);
	strncat(workStr1, (char *)*buffer, WORK_STRING_SIZE);
	ConvertToUpper(workStr1);
	
	/* Show what we were not able to extract							*/
	*hour = *minute = *second = 0;

	bp = workStr1;

	while (isspace(*bp))
		bp++;
	tp = bp;

	*hour = strtol(bp, &bp, 10);
	if (tp == bp || (int)*hour < 0 || *hour > 23)
	{
		ccode |= 1;
		*hour = LONG_MAX;
	}
#ifdef NATIVE
	if (*bp != DOSCountryInfo.timeSep[0])
#else
	/*
	 *	This needs to be made portable!!  I18N
	 */
	if (*bp != ':')
#endif
		goto findAmPm;

	bp++;
	tp = bp;
	*minute = strtol(bp, &bp, 10);
	if (tp == bp || (int)*minute < 0 || *minute > 59)
	{
		ccode |= 2;
		*minute = LONG_MAX;
	}
#ifdef NATIVE
	if (*bp != DOSCountryInfo.timeSep[0])
#else
	/*
	 *	This needs to be made portable!!  I18N
	 */
	if (*bp != ':')
#endif
		goto findAmPm;

	bp++;
	tp = bp;
	*second = strtol(bp, &bp, 10);
	if (tp == bp || (int)*second < 0 || *second > 59)
	{
		ccode |= 4;
		*second = LONG_MAX;
	}

findAmPm:
	
	while (isspace(*bp))
		bp++;

	/* See if the am indicator is present?*/

	memset(workStr2, 0, WORK_STRING_SIZE);
	memset(&tm, 0, sizeof(tm));
	tm.tm_hour = 6;
	if (!strftime(workStr2, WORK_STRING_SIZE, "%p", &tm))
	{
#ifdef DEBUG
		fprintf(stderr, "MsgScanTimeOffset: strftime of am failed\n");
#endif
	}
	ConvertToUpper(workStr2);
	if (bp == strstr(bp, workStr2))
	{
		ampmFlag = 1;
		bp += strlen(workStr2);
		goto finalCheck;
	}

	memset(workStr2, 0, WORK_STRING_SIZE);
	tm.tm_hour = 18;
	if (!strftime(workStr2, WORK_STRING_SIZE, "%p", &tm))
	{
#ifdef DEBUG
		fprintf(stderr, "MsgScanTimeOffset: strftime of pm failed\n");
#endif
	}
	ConvertToUpper(workStr2);
	if (bp == strstr(bp, workStr2))
	{
		ampmFlag = 2;
		bp += strlen(workStr2);
		goto finalCheck;
	}

finalCheck:
	/* If any error has already occurred then hour, minute, or second will*/
	/* be LONG_MAX and won't pass some of these tests.  Just return.*/
	if (ccode != 0)
		goto done;

	/* Check for inconsistent use of am and pm indicator.  Leave the extracted*/
	/* time unchanged.  Maybe the caller will accept it anyway.*/
	switch(ampmFlag)
	{
		case 1:	/* AM */
			if (*hour == 12)
				*hour = 0;
			else if (*hour > 12)
			{
				ccode |= 8;
			}
			break;

		case 2: /* PM */
			if (*hour < 12)
				*hour += 12;
			else if (*hour != 12)
			{
				ccode |= 16;
			}
			break;

		default:
			/* No am or pm indicator was present.  If the country requires*/
			/* a 12 hour clock then it should have been and there is an */
			/* ambiguity.*/
#ifdef NATIVE
/*
 *	This needs to be made portable!!  I18N
 */
			if (DOSCountryInfo.timeFmt == 0)
			{
				ccode |= 32;
			}
#else
			;
#endif
	}

done:

	*buffer += (bp-workStr1); /* Return pointer to the rest of the data*/

#else /* ifndef STRPTIME */

	*buffer = strptime(*buffer, "%X", &tm);
	hour = tm.tm_hour;
	minute = tm.tm_min;
	second = tm.tm_sec;

#endif /* ifndef STRPTIME */

	return ccode;
}

void 
MsgGetTimeStr(char *timeStr)
{
	time_t clock;
	struct tm *tm;

	clock = time(0);
	tm = localtime(&clock);
	strftime(timeStr, 40,"%X", tm); 
}

void
MsgGetDateStr(char *dateStr)
{
	time_t clock;
	struct tm *tm;

	clock = time(0);
	tm = localtime(&clock);
	strftime(dateStr, 40,"%x", tm); 
}

char *
MsgGetStr(int msgNumber)
{
	char *sptr;
	Domain_t	*myDomain;
	unsigned char	*myBuf;

	/* Get domain table */
	if((myDomain = getDomain()) == NULL) {
		return NULL;
	}

#if defined(DEBUG)
	if (myDomain->catd == (nl_catd)-1) {
		fprintf(stderr, "MsgGetStr: MsgBindDomain not called first\n");
		return NULL;
	}
#endif

	sptr = catgets(myDomain->catd, myDomain->set, msgNumber, "");
	if ( *sptr == '\0') {
#if defined(DEBUG)
		fprintf(stderr, "MsgGetStr: pid=%d: file=%d, set=%d, msg=%d, catd=%d: NOT FOUND.\n",
			getpid(), myDomain->fileNumber, myDomain->set, msgNumber, myDomain->catd);
#endif
		return NULL;
	}
	else {
		if((myBuf = getMsgBuf()) == NULL) {
			return NULL;
		}
		(void)strncpy((char *)myBuf, sptr, MSG_MAX_LEN);
		myBuf[MSG_MAX_LEN-1] = '\0';	/* make sure its null */
	}

#if defined(HARD_DEBUG)
	printf("MsgGetStr: Message =%s=\n", myDomain->buf);
#endif

	return ((char *)myBuf);
}

char *
MsgDomainGetStr(int domain, int msgNumber)
{
	int oldDomain;
	char *str;

	oldDomain = MsgGetDomain();
	if(MsgChangeDomain(domain) == SUCCESS) {
		str = MsgGetStr(msgNumber);
	}
	else {
		str = NULL;
	}
	
	MsgChangeDomain(oldDomain);
	return str;
}
