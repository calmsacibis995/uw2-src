/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)proto:cmd/chall.c	1.2"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <sys/types.h>


int
main(int argc, char *argv[])
{
uid_t uid=-1;
gid_t gid=-1;
mode_t perm;
char *mode, *owner, *group, *path, *badchr;
struct group *gr;
struct passwd *pwd;

	if ( argc != 5 ) {
		fprintf(stderr,"ERROR: Usage: %s mode owner group path\n", \
						argv[0]);
		exit(1);
	}

	path=argv[4];

	mode=argv[1];
        perm = (mode_t)strtol(mode, &badchr, 8);
        if (*badchr != '\0' && *badchr != '?') {
		fprintf(stderr,"ERROR: badmode: <%s>\n",mode);
		exit(1);
	} else if ( chmod(path,perm) == -1 ) {
		perror("chmod");
		exit(2);
	}

	owner=argv[2];
	if ( *owner != '?' ) {
		if ((pwd=getpwnam(owner)) == NULL) {
        		uid = (uid_t)strtol(owner, &badchr, 10);
        		if ( *badchr != '\0' ) {
				fprintf(stderr,"ERROR: bad uid: <%s>\n",owner);
				exit(3);
			}
			if(uid > UID_MAX ) {
				fprintf(stderr, "Numeric uid too large\n");
				exit(3);
			}
		} else {
			uid = pwd->pw_uid;
		}
	}

	group=argv[3];
	if ( *group != '?' ) {
		if ((gr = getgrnam(group)) == NULL) {
        		gid = (gid_t)strtol(group, &badchr, 10);
        		if ( *badchr != '\0' ) {
				fprintf(stderr,"ERROR: bad gid: <%s>\n",group);
				exit(4);
			}
			gid = (gid_t)strtol(group,NULL,10);
			if(gid > UID_MAX ) {
				fprintf(stderr, "Numeric gid too large\n");
				exit(4);
			}
		} else {
			gid = gr->gr_gid;
		}
	}

	if ( uid != -1 || gid != -1 )
		if ( chown(path, uid, gid) == -1 ) {
			perror("chown");
			exit(3);
		}
	
	exit(0);

}
