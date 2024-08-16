/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ttymon:common/cmd/ttymon/ttymonexec.c	1.4"

/*
 * ttymonexec.c. this file becomes the front end to the ttymon system.
 *		 it will be installed as 'ttymon' and will call either
 *		 ttymon.tp or ttymon.notp depending on the state of
 *		 the TP_DEFAULT value in /etc/default/tpath 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <deflt.h>
#include <sys/types.h>
#include <pfmt.h>

#define TPCMD "/usr/lib/saf/ttymon.tp"
#define OLDCMD "/usr/lib/saf/ttymon.notp"
#define DEFLT_TPATH_FILE  "tpath"
#define TPNAME "TP_DEFAULT"
#define YES_U	"YES"
#define NO_U	"NO"
#define YES_L	"yes"
#define NO_L	"no"
#define TRUE 	1
#define FALSE   0

extern int errno;
char *Tag;
int	vflag;

main(int argc, char *argv[])
{
	int	B2Running;
	FILE	*tpath_fp;
	char	*tpath_ptr;
	int	tp_on_off;
	char	c;
	char	*cmd;
	int	deamon_mode=0;
	int	force_tp_flag;
	char	*set_lang;
	FILE	*locale_fp;

	/* if locale can't be picked up from the environment, use
	 * the default system locale
	 */
	if ((set_lang = setlocale(LC_ALL, "")) == (char *)NULL ||
	   !strcmp(set_lang, "C"))
		if (((locale_fp = defopen("locale")) != (FILE *)NULL) &&
		   ((set_lang = defread(locale_fp, "LANG")) != (char *)NULL))
			(void)setlocale(LC_ALL, set_lang);
        (void)setcat("uxcore.abi");
        (void)setlabel("UX:ttymon.fe");

	B2Running = is_b2running();

	/* 
	 * PMTAG and log setup are only valid if non-express mode
	 */
	if((argc <= 1) && strcmp(lastname(argv[0]), "getty") ) {
        	if ((Tag = getenv("PMTAG")) == NULL)
                	logexit(1, ":660:PMTAG is missing");
		openlog();
		deamon_mode=1;
	}

	/*
	 * read tpath default file. if file not there or TPATH variable
	 * not set, or it is not 'YES' or 'NO' exit
	 */
	if((tpath_fp = defopen(DEFLT_TPATH_FILE)) == (FILE *)NULL) {
		logexit(1, "::Cannot open %s/%s\n",DEFLT, DEFLT_TPATH_FILE);
	}
	if((tpath_ptr = defread(tpath_fp, TPNAME)) == (char *)NULL) {
		logexit(1, "::Cannot read %s variable from %s/%s\n",TPNAME,DEFLT, DEFLT_TPATH_FILE);
	}
	if((strcmp(tpath_ptr, YES_U) == 0) || (strcmp(tpath_ptr, YES_L) ==0)) {
		tp_on_off = TRUE;
	}
	else if((strcmp(tpath_ptr, NO_U) == 0) || (strcmp(tpath_ptr, NO_L) ==0)) {
		tp_on_off = FALSE;
	}
	else {
		logexit(1, "::unknown value (%s/%s are choices) %s in %s variable from %s/%s\n",YES_U,NO_U,tpath_ptr, TPNAME,DEFLT, DEFLT_TPATH_FILE);
	}

	force_tp_flag = FALSE;

	/*
	 * if we find that B2 is running and TP_DEFAULT=NO we must force
	 * it active. This is a change from previous versions which cause a
	 * fatal error if mac was installed and TP was off. THis caused a
	 * problem if only B1 was installed since only the B2 script forces
	 * tp on. We went to the 'on' model instead of the 'installed' model
	 * by also forcing tp if SAK was on the cmd line (see MR ul93-30225)
	 * for more;
	 */
	if(!tp_on_off && B2Running) {
		force_tp_flag = TRUE;
		log(MM_INFO, "::B2 is running line and TP_DEFAULT!=YES\n%s is being forced to execute.\nYou may wish to run 'defadm tpath TP_DEFAULT=YES'", TPCMD);
	}

	/*
	 * see if -v flag set, if so we would force to TP ttymon. Let
	 * all other opts fall through. (set opterr to 0 so
	 * getopt doesn't complain about these.
	 * Also, if -k or -K on cmd line specifing at 'SAK' key we
	 * force TP ttymon to run because only it understands this option.
	 * see above and MR ul93-30225 for more info.
	 */
	opterr = 0;
	while ((c = getopt(argc, argv, "vgd:ht:p:m:l:k:K:x")) != EOF) {
		switch (c) {
		case 'v':
			force_tp_flag = TRUE;
			break;
		case 'k':
		case 'K':
			if(!tp_on_off && !force_tp_flag) {
				force_tp_flag = TRUE;
				log(MM_INFO, "::SAK key options detected on cmd line and TP_DEFAULT!=YES\n%s is being forced to execute.\nYou may wish to run 'defadm tpath TP_DEFAULT=YES'", TPCMD);
			}
			break;
		default:
			break;
		}
	}



	/*
	 * go on the appropriate 'real' ttymon.
	 * if specified TPATH=YES, or we somehow forced it above, go on to
	 * the (TP) ttymon, else go to the ttymon that doesn't support
	 */
	if(tp_on_off || force_tp_flag) {
		cmd = TPCMD;
	}
	else {
		cmd = OLDCMD;
	}

	/*
	 * we'll call exec without reseting argv[0] to the command we
	 * are to exec (ttymon.tp or ttymon.notp) for two reasons. First
	 * if we came here on a sym link from /etc/getty we need argv to
	 * still have 'getty' because the real ttymon uses that knowledge.
	 * second. a 'ps' would show ttymon running an not ttymon.tp (or
	 * ttymon.notp).
	 */
	
	if (deamon_mode)
		log(MM_INFO, "::Starting %s:", cmd);
			
	(void)execv(cmd, argv);

	/* exec returns only on failure */
	logexit(1, "::Cannot execute %s", argv[0], strerror(errno));
}
