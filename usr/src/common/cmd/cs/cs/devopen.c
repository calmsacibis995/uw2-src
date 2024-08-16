/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*              copyright       "%c%"   */

#ident	"@(#)cs:cs/devopen.c	1.3"
#ident	"$Header: $"

#include <unistd.h>
#include "uucp.h"
#include <sys/stropts.h>
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/tp.h>
#include <stdio.h>

static  size_t tp_errno;
static  char tp_ebuf[80];

int
dev_open(dcname,flags)
char *dcname;
int flags;
{
	int fd;
	struct tp_info	tpinfo;

        if (no_tp_wanted(dcname)) {
       		if ((fd = open(dcname,flags)) < 0) {
            		DEBUG(5, gettxt(":299", "cs-tp: (trusted path) open error: %s\n"), strerror(errno));
            		DEBUG(5, "cs-tp: open errno=%d\n", errno);
            		return(-1);
          	}
        } else {
		if ((fd = tp_open(dcname, flags)) < 0){
			if (errno == EINVAL) {
				/*  Check for EINVAL to determine if 
			 	*  the device has a clist driver.
			 	*  This check is heuristic. Typically EINVAL
			 	*  is returned from tp_open() when it attempts
			 	*  to link (i.e. ioctl(I_PLINK) ) the physical 
			 	*  device "dcname" under a TP device and "dcname" 
			 	*  is not a STREAMS based device. If EINVAL is 
			 	*  returned we assume that I_PLINK failed because
			 	*  "dcname" is clist based device and open it via 
			 	*  a standard open(2) interface.
			 	*/

				if ((fd = open(dcname,flags)) < 0) {
					DEBUG(5, gettxt(":299", "cs-tp: (trusted path) open error: %s\n"), strerror(errno));
					DEBUG(5, "cs-tp: open errno=%d\n", errno);
					return(-1);
				}
			} else {
				tp_geterror(tp_errno, (size_t)80, tp_ebuf);
				DEBUG(5, gettxt(":300", "cs-tp: (trusted path) tp_open error: %s\n"), strerror(errno));
				DEBUG(5, "cs-tp: tp_open errno=%d\n", errno);
				DEBUG(5, "cs-tp: tp_ebuf %s\n", tp_ebuf);
				return(-1);
			}
		}
	}

	return(fd);
}


/*
 * Procedure:   no_tp_wanted
 *
 *      Reads the tp_config default file to determine whether or not
 *      a "trusted path" connection should be established for the
 *      device passed as an argument.
 */

#define CS_TPCONFIGFILE    "cs_tpconfig"
#define TPATH_FILE    "tpath"

static  int
no_tp_wanted(dcname)
        char *dcname;
{
        FILE *fp;
        char *p;
        int     ret = 0;
        int     found = 0;
        extern  FILE    *defopen();
        extern  char    *defread();

        DUMP((msg,gettxt(":301", "cs-tp: (trusted path) device name <%s>"), dcname));
        if ((fp = defopen(CS_TPCONFIGFILE)) != NULL) {
                if ((p = defread(fp, dcname)) != NULL) {
                        if (*p) {
                                found = 1;
                                DUMP((msg,gettxt(":302", "cs-tp: (trusted path) action <%s>"), p));
                                if (strcmp(p, "no") == 0)
                                        ret = 1;
                                else if (strcmp(p, "NO") == 0)
                                        ret = 1;
                        }
                }
                (void) defclose(fp);
        }
        if (! found) {
                if ((fp = defopen(TPATH_FILE)) != NULL) {
                        if ((p = defread(fp, "TP_DEFAULT")) != NULL) {
                                if (*p) {
                                        DUMP((msg,gettxt(":303", "cs-tp: (trusted path) TP_DEFAULT=%s"), p));
                                        if (strcmp(p, "no") == 0)
                                                ret = 1;
                                        else if (strcmp(p, "NO") == 0)
                                                ret = 1;
                                }
                        }
                        (void) defclose(fp);
                }
        }

        return ret;
}

