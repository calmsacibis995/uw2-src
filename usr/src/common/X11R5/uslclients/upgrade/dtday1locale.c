/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtupgrade:dtday1locale.c	1.1"


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <unistd.h>
#include <errno.h>

#define DAYONE_DIR "/usr/X/desktop/LoginMgr/DayOne"

main(int argc, char **argv)
{
    char *app_name = argv[0];
    char *login_id = argv[1];
    char filename[PATH_MAX];
    char locale_name[PATH_MAX];
    size_t bytes_in;
    int	fd;
    uid_t  real_uid;
    struct passwd *password;

    if (argc != 3){
	fprintf(stderr,"Usage: %s: <login> <locale>\n", app_name);
	exit(1);
    }

    sprintf(filename,"%s/%s", DAYONE_DIR, login_id);
    sprintf(locale_name,"%s\n", argv[2]);

    real_uid = getuid();
    if (real_uid != 0){
	password = getpwuid(real_uid);
	if (strcmp(password->pw_name, login_id)){
	    fprintf(stderr,"%s: %s cannot update DayOne file for %s\n",
		    app_name, password->pw_name, login_id);
	    exit(1);
	}
    }

    errno = 0;
    if ((fd = open(filename, O_CREAT | O_EXCL | O_WRONLY)) != -1){
	bytes_in = (size_t) strlen(locale_name);
	if (write(fd, locale_name, bytes_in) != bytes_in){
	    fprintf(stderr,"%s: failed to write DayOne locale file\n", 
		    app_name);
	    close(fd);
	    unlink(filename);
	    exit(1);
	}
	else{
	    /* make file readable by all */
	    fchmod(fd, S_IRUSR | S_IRGRP | S_IROTH);
	    close(fd);
	    exit(0);
	}

    }
    else{
	if (errno == EEXIST)
	    fprintf(stderr,"%s: DayOne locale file exists for %s\n", 
		    app_name, login_id);
	else
	    fprintf(stderr,"%s: failed to open DayOne locale file\n", 
		    app_name);
	exit(1);
    }
}
