/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamintf:common/cmd/oamintf/usermgmt/lsgrp.c	1.1.6.3"
#ident  "$Header: lsgrp.c 2.0 91/07/12 $"

#include <stdio.h>
#include <pwd.h>
#include <grp.h>

struct group *getgrent();
struct passwd *getpwent();
char *strcat();
void setpwent();

#define MINUSRID	100

main()
{
	struct group *gstruct;		/* ret from getgrnam */
	struct passwd *pstruct;		/* ret from getpwent */
	char prmgrps[BUFSIZ];		/* buffer for primary group members */
	char **pp;			/* ptr to supplememtary grp member */
	int added;			/* check flag for group added */
	int firsttime;			/* first time checker */
	int x;				/* throw away variable */
	int size_used,user_name_size;   /* Integers used to ensure we don't
                                           strcat into a full buffer */
	

	/* Get group entry and check for user ID (greater than 99) */
	while((gstruct = getgrent()) != NULL) {
		if((gstruct->gr_gid < MINUSRID) && (strcmp(gstruct->gr_name, "other") != 0)) {
			continue;
		}

		/* Clear primary group buffer */
		for (x=0; x < BUFSIZ; prmgrps[x++]='\0')
		added = 0;
		size_used=1;

                printf("Group Name:              %s\n",gstruct->gr_name);
                printf("Group ID:                %d\n",gstruct->gr_gid);
                printf("Primary Members:         ");

		/* Buffer primary group members */
		while((pstruct = getpwent()) != NULL) {
			if (pstruct->pw_gid == gstruct->gr_gid) {
				user_name_size=strlen(pstruct->pw_name);
                                if ((user_name_size + size_used + 1) >= BUFSIZ){                                        printf("%s",prmgrps);
                                        for (x=0; x < BUFSIZ; prmgrps[x++]='\0');
                                        size_used=1;
                                }
                                size_used=size_used + user_name_size + 1;
				if(added) strcat(prmgrps, ",");
				strcat(prmgrps, pstruct->pw_name);
				added = 1;
			}
		}

		/* rewind password file */
		setpwent();

		printf("%s\n",prmgrps);
    		printf("Supplementary members:   ");
		
		firsttime=1;
		for (pp = gstruct->gr_mem; *pp != (char *) NULL; pp++) {
			if (*pp != (char *) NULL && firsttime) {
    				printf("%s",*pp);
				firsttime=0;
			}
			else if (*pp != (char *) NULL && !firsttime) {
    				printf(",%s",*pp);
			}
		}	
    		printf("\n\n");
	}
}
