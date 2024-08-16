/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/extra.c	1.1.3.2"
#ident "@(#)extra.c	1.3 'attmail mail(1) command'"

#include <stdio.h>
#include "smtp.h"
#include "mail.h"

char *full_domain_name(domain)
char *domain;
{
	static string *fullname = 0;
	if (fullname) s_free(fullname);
	fullname = s_xappend((string*)0, mailsystem(0), domain ? domain : maildomain(), (char*)0);
	return s_to_c(fullname);
}

char *message_id()
{
	register struct tm *t;
	static char msgid[128];
	static time_t was = (time_t) 0;
	static int uniqueval;
	char unique[3];
	time_t now;

	now = time((long *) 0);
	t = localtime(&now);
	if ((now/60) == (was/60)) {
		uniqueval++;
	} else {
		was = now;
		uniqueval = 0;
	}
	unique[0] = 'A' + (uniqueval / 26);
	unique[1] = 'A' + (uniqueval % 26);
	unique[2] = '\0';
	sprintf(msgid, "<%2.2d%2.2d%2.2d%2.2d%2.2d.%2.2s%5.5d@%.100s>",
		t->tm_year, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min,
		unique, getpid(), full_domain_name((char *)NULL));
	return msgid;
}
