/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsocket:common/lib/libsocket/inet/ifignore.c	1.1.1.3"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */

#include "../socketabi.h"
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include "../libsock_mt.h"

static struct ifiginfo {
	FILE	*ifigf;
	char	*_ifig_file;
	char	 line[BUFSIZ+1];
} ifiginfo = { NULL, "/etc/inet/if.ignore" };

static struct ifiginfo *
get_s_ifiginfo()
{

#ifdef _REENTRANT
        struct _s_tsd *key_tbl;
	struct ifiginfo *iip;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		return (&ifiginfo);
	} 
	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _s_tsd *)
		  _mt_get_thr_specific_storage(_s_key, _S_KEYTBL_SIZE);
	if (key_tbl == NULL)
		return ((struct ifiginfo *)NULL);
	if (key_tbl->ifig_info_p == NULL) {
		iip = (struct ifiginfo *)calloc(1, sizeof(struct ifiginfo));
		if (iip == NULL)
			return ((struct ifiginfo *)NULL);
		key_tbl->ifig_info_p = iip;
		iip->_ifig_file = "/etc/inet/if.ignore";
	}
	
	return ((struct ifiginfo *)key_tbl->ifig_info_p);
#else /* !_REENTRANT */
	return (&ifiginfo);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_s_ifig_info(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

setifigfile(file)
	char *file;
{
	struct ifiginfo *iip;

	/* Get thread-specific data */
	if ((iip = get_s_ifiginfo()) == NULL)
		return (NULL);

	iip->_ifig_file = file;
}

ifignore(ifname, svrname)
char	*ifname, *svrname;
{
	register char	*p;
	register int	 n;
	struct ifiginfo *iip;
	char 		*lasts;

	/* Get thread-specific data */
	if ((iip = get_s_ifiginfo()) == NULL)
		return (0);


	if (!(iip->ifigf = fopen(iip->_ifig_file, "r" )))
		return(0);
 	if (!ifname || !*ifname) {
		fclose(iip->ifigf);
		return(0);
	}
	while (fgets(iip->line, BUFSIZ, iip->ifigf)) {
		p = strtok_r(iip->line, " \t\n", &lasts);
		if (!p || *p == '#' || strcmp(p, ifname) != 0)
			continue;
		if (!svrname || !*svrname) {
			fclose(iip->ifigf);
			return(1);
		}
		n = 0;
		while (p = strtok_r(NULL, " \t\n", &lasts)) {
			if (*p == '#') {
				p = (char *)NULL;
				break;
			}
			n++;
			if (strcmp(p, svrname) == 0)
				break;
		}
		if (p || !n) {
			fclose(iip->ifigf);
			return(1);
		}
	}
	fclose(iip->ifigf);
	return(0);
}
