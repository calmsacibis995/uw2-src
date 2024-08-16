/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libpkg:common/lib/libpkg/ckparam.c	1.2.1.5"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/lib/libpkg/ckparam.c,v 1.1 91/02/28 20:55:01 ccs Exp $"

#include <ctype.h>
#include <string.h>
#include <sys/types.h>

extern void	progerr();

#define ERR_LEN		"uxpkgtools:596:length of parameter <%s> exceeds limit"
#define ERR_ASCII	"uxpkgtools:597:parameter <%s> must be ascii"
#define ERR_ALNUM	"uxpkgtools:598:parameter <%s> must be alphanumeric"	
#define ERR_CHAR	"uxpkgtools:599:parameter <%s> has incorrect first character"
#define ERR_UNDEF	"uxpkgtools:600:parameter <%s> cannot be null"

#define MAXLEN 256
#define TOKLEN 16

static int	cklen(), ckascii(), ckalnum();

ckparam(param, value)
char *param, *value;
{
	char *token,*tmpvalue;

	tmpvalue = (char *)malloc(strlen(value)+1);
	(void) strcpy(tmpvalue,value);

	if(strcmp(param, "NAME") == 0) {
		if(cknull(value)) {
			progerr(ERR_UNDEF, param);
			return(1);
		}
		if(cklen(value, MAXLEN)) {
			progerr(ERR_LEN, param);
			return(1);
		}
		if(ckascii(value)) {
			progerr(ERR_ASCII, param);
			return(1);
		}
		return(0);
	}
	if(strcmp(param, "ARCH") == 0) {
		if(cknull(value)) {
			progerr(ERR_UNDEF, param);
			return(1);
		}
		token = strtok(tmpvalue, ",");
		while(token) { 
			if(cklen(token, TOKLEN)) {
				progerr(ERR_LEN, param);
				return(1);
			}
			if(ckascii(token)) {
				progerr(ERR_ASCII, param);
				return(1);
			}
			token = strtok(NULL, ",");
		}
		return(0);
	}
	if(strcmp(param, "VERSION") == 0) {
		if(cknull(value)) {
			progerr(ERR_UNDEF, param);
			return(1);
		}
		if(*value == '(') {
			progerr(ERR_CHAR, param);
			return(1);
		}
		if(cklen(value, MAXLEN)) {
			progerr(ERR_LEN, param);
			return(1);
		}
		if(ckascii(value)) {
			progerr(ERR_ASCII, param);
			return(1);
		}
		return(0);
	}
	if(strcmp(param, "CATEGORY") == 0) {
		if(cknull(value)) {
			progerr(ERR_UNDEF, param);
			return(1);
		}
		token = strtok(tmpvalue, ",");
		while(token) { 
			if(cklen(token, TOKLEN)) {
				progerr(ERR_LEN, param);
				return(1);
			}
			if(ckalnum(token)) {
				progerr(ERR_ALNUM, param);
				return(1);
			}
			token = strtok(NULL, ",");
		}
		return(0);
	}
	/* param does not match existing parameters */
	return(1);
}

int 
cknull(pt)
char *pt;
{
	int length;

	if((length = strlen(pt)) == 0)
		return(1);
	return(0);
}	

int
cklen(pt, len)
char *pt;
int len;
{
	int length;

	if((length = strlen(pt)) > len)
		return(1);
	return(0);
}

int
ckascii(pt)
char *pt;
{
	while(*pt) { 
		if(!(isascii(*pt))) 
			return(1);
		pt++;
	}
	return(0);
}

int
ckalnum(pt)
char *pt;
{
	while(*pt) {
		if(!(isalnum(*pt))) 
			return(1);
		pt++;
	}
	
	return(0);
}
