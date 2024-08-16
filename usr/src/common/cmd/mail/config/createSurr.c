/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/config/createSurr.c	1.4"
#include	<stdio.h>
#include	<pwd.h>
#include	<grp.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<string.h>

#define	CONFIG_FILES	"/etc/mail/config.files"

extern char
    *optarg;

int
main(int argc, char **argv)
    {
    FILE
	*fp_fileList;

    struct passwd
	*userPwd;
    
    struct group
	*groupGrp;

    uid_t
	userId;
    
    gid_t
	groupId;

    mode_t
	mask;

    int
	result = 0,
	debug = 0,
	mode,
	c;
    
    char
	*phase = "01",
	*destFile,
	*protoFile,
	*userName,
	*groupName,
	*modeStr,
	*p,
	buffer[1024],
	checkBuff[1024];

    strcpy(checkBuff, "/usr/lib/mail/surrcmd/configCheck");
    while((c = getopt(argc, argv, "d:l:p:s:")) != EOF)
	{
	switch(c)
	    {
	    case	'd':
		{
		debug = atoi(optarg);
		break;
		}

	    case	'l':
		{
		for
		    (
		    p = optarg;
		    *p != '\0';
		    p++
		    )
		    {
		    *p = toupper(*p);
		    }

		if(debug) fprintf(stderr, "-l \"%s\"\n", optarg);
		if(strcmp(optarg, "ON"))
		    {
		    strcat(checkBuff, " -l 0");
		    }
		else
		    {
		    strcat(checkBuff, " -l 1");
		    }

		break;
		}

	    case	'p':
		{
		phase = optarg;
		break;
		}

	    case	's':
		{
		for
		    (
		    p = optarg;
		    *p != '\0';
		    p++
		    )
		    {
		    *p = toupper(*p);
		    }

		if(debug) fprintf(stderr, "-s \"%s\"\n", optarg);
		if(strcmp(optarg, "ON"))
		    {
		    strcat(checkBuff, " -s 0");
		    }
		else
		    {
		    strcat(checkBuff, " -s 1");
		    }

		break;
		}
	    }
	}

    while((c = *phase++) != '\0')
	{
	switch(c)
	    {
	    default:
		{
		break;
		}

	    case	'0':
		{
		system(checkBuff);
		break;
		}

	    case	'1':
		{
		if((fp_fileList = fopen(CONFIG_FILES, "r")) == NULL)
		    {
		    perror(CONFIG_FILES);
		    result = 1;
		    }
		else while(fgets(buffer, sizeof(buffer), fp_fileList) != NULL)
		    {
		    if((destFile = strtok(buffer, " \t\r\n")) == NULL)
			{
			}
		    else if((protoFile = strtok(NULL, " \t\r\n")) == NULL)
			{
			}
		    else if((userName = strtok(NULL, " \t\r\n")) == NULL)
			{
			}
		    else if((groupName = strtok(NULL, " \t\r\n")) == NULL)
			{
			}
		    else if((modeStr = strtok(NULL, " \t\r\n")) == NULL)
			{
			}
		    else if((userPwd = getpwnam(userName)) == NULL)
			{
			}
		    else if((groupGrp = getgrnam(groupName)) == NULL)
			{
			}
		    else if((setuid (userPwd->pw_uid)) == -1)
			{
			}
		    else if((setgid (groupGrp->gr_gid)) == -1)
			{
			}
		    else
			{
			sprintf
			    (
			    checkBuff,
			    "/usr/lib/mail/surrcmd/configFiles %s %s",
			    destFile,
			    protoFile
			    );

			if(debug) fprintf(stderr, "%s\n", checkBuff);
			mask = (~strtol(modeStr, NULL, 0))&0xfff;
			umask(mask);
			system(checkBuff);
			}
		    }

		break;
		}
	    }
	}

    return(result);
    }
