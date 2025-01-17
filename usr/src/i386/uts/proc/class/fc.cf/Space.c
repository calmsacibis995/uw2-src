/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:proc/class/fc.cf/Space.c	1.2"

#include <config.h>
#include <sys/types.h>
#include <sys/fcpriocntl.h>
#include <sys/fc.h>

short	fc_maxupri=FCMAXUPRI;
int	fc_affinity_on=FC_AFFINITY_ON;

fcdpent_t	fc_dptbl[] = {
				   0,   100,
				   1,   100,
				   2,   100,
				   3,   100,
				   4,   100,
				   5,   100,
				   6,   100,
				   7,   100,
				   8,   100,
				   9,   100,
				  10,   80,
				  11,   80,
				  12,   80,
				  13,   80,
				  14,   80,
				  15,   80,
				  16,   80,
				  17,   80,
				  18,   80,
				  19,   80,
				  20,   60,
				  21,   60,
				  22,   60,
				  23,   60,
				  24,   60,
				  25,   60,
				  26,   60,
				  27,   60,
				  28,   60,
				  29,   60,
				  30,   40,
				  31,   40,
				  32,   40,
				  33,   40,
				  34,   40,
				  35,   40,
				  36,   40,
				  37,   40,
				  38,   40,
				  39,   40,
				  40,   20,
				  41,   20,
				  42,   20,
				  43,   20,
				  44,   20,
				  45,   20,
				  46,   20,
				  47,   20,
				  48,   20,
				  49,   20,
				  50,    10,
				  51,    10,
				  52,    10,
				  53,    10,
				  54,    10,
				  55,    10,
				  56,    10,
				  57,    10,
				  58,    10,
				  59,    10,
				  };

int	fc_kmdpris[]
		      = {
			60,61,62,63,64,65,66,67,68,69,
			70,71,72,73,74,75,76,77,78,79,
			80,81,82,83,84,85,86,87,88,89,
			90,91,92,93,94,95,96,97,98,99
			};

short	fc_maxkmdpri = sizeof(fc_kmdpris)/4 - 1;
short	fc_maxumdpri = sizeof(fc_dptbl)/8 - 1;
