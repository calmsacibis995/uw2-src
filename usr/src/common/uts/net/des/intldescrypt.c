/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/des/intldescrypt.c	1.4"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	intldescrypt.c, international version of DES encryption
 *	library routines.
 */


#include <util/types.h>
#include <net/rpc/des_crypt.h>
#include <net/des/des.h>

/*
 * Copy 8 bytes
 */
#define COPY8(src, dst) { \
}
 
/*
 * Copy multiple of 8 bytes
 */
#define DESCOPY(src, dst, len) { \
	} \
}

/*
 * cbc_crypt(key, buf, len, mode, ivec)
 *	CBC mode encryption.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0 on success and positive error on failure.
 *
 * Description:
 *	Software encrypt or decrypt a block of data using
 *	cypher block chaining mode.
 *
 *	This international version does nothing.
 *
 * Parameters:
 *
 *	key			# key for encryption
 *	buf			# data to process
 *	len			# length of data
 *	mode			# encrypt or decryt.
 *	ivec			# 
 *	
 */
/* ARGSUSED */
int
cbc_crypt(char *key, char *buf, unsigned len, unsigned mode, char *ivec)
{
	return(0);
}


/*
 * ecb_crypt(key, buf, len, mode)
 *	ECB mode encryption.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0 on success and positive error on failure.
 *
 * Description:
 *	Software encrypt or decrypt a block of data using
 *	electronic code book mode.
 *
 *	This international version does nothing.
 *
 * Parameters:
 *
 *	key			# key for encryption
 *	buf			# data to process
 *	len			# length of data
 *	mode			# encrypt or decryt.
 *
 */
/* ARGSUSED */
int
ecb_crypt(char *key, char *buf, unsigned len, unsigned mode)
{
	return(0);
}


/*
 * common_crypt(key, buf, len, mode, desp)
 *	 Common code to cbc_crypt() & ecb_crypt()
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0 on success and positive error on failure.
 *
 * Description:
 *	Common code to cbc_crypt() & ecb_crypt().
 *
 *	This international version does nothing.
 *
 * Parameters:
 *
 *	key			# key for encryption
 *	buf			# data to process
 *	len			# length of data
 *	mode			# encrypt or decryt.
 *	desp			#
 *
 */
/* ARGSUSED */
int
common_crypt(char *key, char *buf, unsigned len,
	     unsigned mode, struct desparams *desp)	
{
	return(0);
}
