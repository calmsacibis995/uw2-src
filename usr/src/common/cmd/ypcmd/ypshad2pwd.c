/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ypcmd:ypshad2pwd.c	1.5"
#ident  "$Header: $"
#ifndef lint
static char SysVr3NFSID[] = "@(#)ypshad2pwd.c	5.1 System V NFS source";
#endif /* lint */
/*      SCCS IDENTIFICATION        */
#include <unistd.h>
#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <pwd.h>
#include <shadow.h>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <errno.h>
#include <sys/stat.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

/*
 * Usuage:
 * "ypshad2pwd inpasswdfile inshadowfile outpasswdfile"
 *
 * ypshad2pwd will take the inpasswdfile and merge it with the inshadowfile, 
 * if inshadowfile exists, and place the result in outpasswdfile.
 * If inshadowfile does not exist, it just copies inpasswdfile to outpasswdfile
 */

char *temp = "/etc/ptmp";	/* lockfile for modifying 'file' */
char *inpwfile,*inshadowfile,*outpwfile;
int   use_shadow = TRUE;
FILE *shdfp = NULL;

init(argc,argv)
int argc;
char **argv;
{
	
	if (argc < 3) {
	    	pfmt(stderr, MM_STD, ":1:usage: %s inpwdfile inshadowfile [outpwdfile]\n", argv[0]);
		exit(1);
	}
	inpwfile = argv[1];
	inshadowfile = argv[2];
	outpwfile = argv[3];

	/* make sure we can read the input passwd file */
	if (access(inpwfile, 04) < 0) {
		pfmt(stderr, MM_STD,  ":2:can't read %s\n", inpwfile);
		exit(1);
	}

	/* If the shadow file does not exist, thats ok,
 	 * but if it does and we can't read it, then
	 * that is an error
	 */

	if (access(inshadowfile, 04) < 0) {
		if (errno == ENOENT) {
			use_shadow=FALSE;
		} else{
			pfmt(stderr, MM_STD, ":3:error %d: can't read %s\n",
				errno, inshadowfile);
			exit(1);
		}
	}

	/* If no out passwd file is given, used stdout, 
	 * if the out passwd file exists, we should be able to write it,
	 * if it doesn't exist thats ok, the rename below will create it
 	 */

	if ( outpwfile != NULL && access(outpwfile, 02)  < 0) {
		if (errno != ENOENT) {
			pfmt(stderr, MM_STD,
				":4:error %d: can't write %s\n", errno, outpwfile);
			exit(1);
		}
	}
}

struct spwd *
fgetspnam(name)
char	*name ;
{
	register struct spwd *p = (struct spwd *)0;

	if (shdfp == NULL) {
		if ((shdfp = fopen(inshadowfile, "r")) == NULL)
			return(p);	
	}
	rewind(shdfp);
	while ((p = fgetspent(shdfp)) && strcmp(name, p->sp_namp))
		;
	return(p);
}

main(argc,argv)
int argc;
char **argv;
{

	int tempfd;
	FILE *tempfp,*inpw,*inshadow,*pfile;
	void (*f1)(), (*f2)(), (*f3)();
	struct passwd *pw;
	struct spwd *spw;
	extern struct passwd* fgetpwent();

        (void)setlocale(LC_ALL,"");
        (void)setcat("uxypshad2pwd");
        (void)setlabel("UX:ypshad2pwd");

	init(argc,argv);

	(void) umask(0);

	f1 = signal(SIGHUP, SIG_IGN);
	f2 = signal(SIGINT, SIG_IGN);
	f3 = signal(SIGQUIT, SIG_IGN);

	if (outpwfile != NULL){
		tempfd = open(temp, O_WRONLY|O_CREAT|O_EXCL, 0644);
		if (tempfd < 0) {
			if (errno == EEXIST)
				pfmt(stderr, MM_STD, ":5:password file busy - try again.\n");
			else
				pfmt(stderr, MM_NOGET, "%s\n", strerror(errno));
			goto cleanup;
		}
		if ((tempfp = fdopen(tempfd, "w")) == NULL) {
			pfmt(stderr, MM_STD, ":6:fopen failed on %d\n",tempfd);
			close(tempfd);
			unlink(temp);
			exit(1);
		}
	} else
		tempfp = stdout;

	if ((pfile= fopen(inpwfile, "r")) == NULL) {
		pfmt(stderr, MM_STD, ":6:fopen failed on %d\n",inpwfile);
		fclose(tempfp);
		unlink(temp);
		exit(1);
	}
	while( (pw =fgetpwent(pfile)) != NULL) {
		/*
		 * this allows +:::: entry 
		 * anywhere in /etc/passwd, as on SunOs4.1 
		 */
		if(!strcmp(pw->pw_name, "+"))
                        continue;

		if (use_shadow) {
			spw = fgetspnam(pw->pw_name);
			if (spw == NULL)  {
				pfmt(stderr, MM_STD, ":7:no shadow passwd entry for %s\n", 
				pw->pw_name);
				if (shdfp)
					(void)fclose(shdfp);
				(void)fclose(pfile);
				(void)fclose(tempfp);
				unlink(temp);
				exit(1);
			}

			fprintf(tempfp, "%s:%s:%d:%d:%s:%s:%s\n",
				pw->pw_name,
				spw->sp_pwdp,  /* user password */
				pw->pw_uid,
				pw->pw_gid,
				pw->pw_gecos,
				pw->pw_dir,
				pw->pw_shell);

		} else {
			fprintf(tempfp, "%s:%s:%d:%d:%s:%s:%s\n",
				pw->pw_name,
				pw->pw_passwd,  /* user password */
				pw->pw_uid,
				pw->pw_gid,
				pw->pw_gecos,
				pw->pw_dir,
				pw->pw_shell);
		}

		
	}
	if (shdfp)
		(void)fclose(shdfp);
	(void)fclose(pfile);
	if (tempfp != stdout) {
		(void)fclose(tempfp);
		if (rename(temp, outpwfile) < 0) {
			pfmt(stderr, MM_NOGET, "%s: %s\n",
				gettxt(":9", "rename"),
					strerror(errno));
			unlink(temp);
		}
	}
cleanup:
	signal(SIGHUP, f1);
	signal(SIGINT, f2);
	signal(SIGQUIT, f3);

	exit(0);
}
