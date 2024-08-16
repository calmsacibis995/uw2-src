/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:i386at/ktool/idtools/idtarg/bus.c	1.1"
#ident	"$Header:"

#include	<stdio.h>
#include	<unistd.h>
#include	<errno.h>
#include	<sys/types.h>
#include	<sys/fcntl.h>
#include	<sys/sysi86.h>


bus_name()
{
	char	name[5];
	int	bustype;
	int	fd;


	if ((bustype = sysi86(SI86BUSTYPE)) < 0)
		exit(errno);

	switch(bustype)	{
	case ISA_BUS:

		/* kuldge to look for "EISA" in the ROM BIOS */
		if ((fd = open("/dev/mem", O_RDONLY)) < 0)
			exit(errno);

		if (lseek (fd, 0xfffd9, SEEK_SET) < 0)
			exit(errno);

		if (read(fd, name, sizeof(name)) != sizeof(name))
			exit(errno);

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
		exit(255);
	}

	exit(0);
}
