/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)skey:validate.c	1.2"
#ident  "$Header: $"

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/keyctl.h>


int
main(int argc, char *argv[])
{
	static k_skey_t	skeys; 
	int		error;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s Serial_id  Serial_key\n", argv[0]);
		return 1;
	}

	if (strlen(argv[1]) > STRLEN || strlen(argv[2]) > STRLEN) {
		fprintf(stderr,
			"Key pairs can not be larger than %d characters.\n",
			STRLEN);
		return 1;
	}

	/*
	 * get the serialnum-id string
	 */
	strncpy((char *)skeys.sernum, argv[1], strlen(argv[1]));

	/*
	 * get the serial key string.
	 */
	strncpy((char *)skeys.serkey, argv[2], strlen(argv[2]));

	keyctl(K_VALIDATE, &skeys, 1);
	error = errno;

	return error;
}
