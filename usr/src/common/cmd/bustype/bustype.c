/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1993 UNIVEL	*/

#ident	"@(#)bustype:bustype.c	1.1"

#include	<stdio.h>
#include	<unistd.h>
#include	<errno.h>
#include	<sys/types.h>
#include	<sys/fcntl.h>
#include	<sys/sysi86.h>

extern char	*sys_errlist[];
extern off_t	lseek(int, off_t, int);


main(argc, argv)
int	argc;
char	*argv[];
{
	char	name[5];
	int	bustype;
	int	fd;


	if ((bustype = sysi86(SI86BUSTYPE)) < 0)	{
		fprintf(stderr, "%s: sysi86 SI86BUSTYPE failed: <%s>\n",
			argv[0], sys_errlist[errno]);
		exit(1);
	}

	switch(bustype)	{
	case ISA_BUS:

		/* kuldge to look for "EISA" in the ROM BIOS */
		if ((fd = open("/dev/mem", O_RDONLY)) < 0)	{
			fprintf(stderr, "%s: open /dev/mem failed: <%s>\n",
					argv[0], sys_errlist[errno]);
			exit(2);
		}
		if (lseek (fd, 0xfffd9, SEEK_SET) < 0)	{
			fprintf(stderr,
				"%s: seek /dev/mem 0xfffd9 failed: <%s>\n",
				argv[0], sys_errlist[errno]);
			exit(3);
		}

		errno = 0;
		if (read(fd, name, sizeof(name)) != sizeof(name))	{
			fprintf(stderr,
				"%s: read /dev/mem 0xfffd9 failed: <%s>\n",
				argv[0], errno ? sys_errlist[errno] :
						"Couldn't read 4 characters");
			exit(4);
		}
		if (strncmp(name, "EISA", 4) == 0)	{
			printf("EISA\n");
			break;
		}

		printf("ISA\n");
		break;

	case MCA_BUS:
		printf("MCA\n");
		break;

	case EISA_BUS:
		printf("EISA\n");
		break;

	default:
		fprintf(stderr, "%s: invalid bus type: 0x%x\n", argv[0], bustype);
		exit(5);
	}

	exit(0);
}

