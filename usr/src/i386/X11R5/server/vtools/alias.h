/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vtools:alias.h	1.5"

/*
 *	Copyright (c) 1992, 1993 USL
 *	Copyright (c) 1994 Novell
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF NOVELL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */



#define MAX_ALIASES 20

typedef struct _alias_list
{
	char *chipset_selected;
	char *compatible_chipset_names[MAX_ALIASES];
}alias_list;

const alias_list aliases[]=
{

{ "MACH64",     { "ATI Mach-64", NULL }                 },
{ "Mach32", 	{ "ATI Mach-32", NULL }					},
{ "Mach8", 		{ "ATI Mach-8", "ATI Mach-32", NULL } 			},
{ "S3928", 		{\
		  "S3 86C928, A,B,C, or D-step", "S3 86C928, E-step",\
		  "S3 86C928PCI" , "S3 86C928, G-step", NULL }				},
{ "S3928/801", 	{\
		  "S3 86C801, A or B-step", \
		  "S3 86C801, C-step", "S3 86C801, D-step",\
		  "S3 86C805, A or B-step", \
		  "S3 86C805, C-step", "S3 86C805, D-step",\
		  "S3 86C928, A,B,C, or D-step", "S3 86C928, E-step",\
		  "S3 86C928PCI" , "S3 86C928, G-step", NULL }				},
{ "S3801", 		{\
		  "S3 86C801, A or B-step", \
		  "S3 86C801, C-step", "S3 86C801, D-step",\
		  "S3 86C805, A or B-step", \
		  "S3 86C805, C-step", "S3 86C805, D-step",\
		  "S3 86C928, A,B,C, or D-step", "S3 86C928, E-step",\
		  "S3 86C928PCI" , "S3 86C928, G-step", NULL }				},

{ "Vision964", {\
		   "S3 Vision964", NULL }                          },
{ "Vision864", {\
		  "S3 Vision864", NULL }                          },
{ "GD54xx",	 	{\
		  "Cirrus CL-GD5402", "Cirrus CL-GD5402 Rev 1",\
		  "Cirrus CL-GD5420", "Cirrus CL-GD5420 Rev 1",\
		  "Cirrus CL-GD5422", "Cirrus CL-GD5424", \
		  "Cirrus CL-GD5426", "Cirrus CL-GD5428",\
		  "Cirrus CL-GD5434", NULL }								 },
{ "ET4000", 	{\
		  "Tseng ET3000", "Tseng ET4000", NULL } },
{ "ETW32", 	{\
		   "Tseng ET4000/W32","Tseng ET4000/W32i",\
           "Tseng ET4000/W32i", "Tseng ET4000/W32i Revision B", NULL } },
{ "ETW32p", 	{\
           "Tseng ET4000/W32p Revision A", "Tseng ET4000/W32p Revision B",\
           "Tseng ET4000/W32p Revision C", NULL } },
{ "77C22E",		{\
		  "NCR 77C21", "NCR 77C22", "NCR 77C22E", "NCR 77C22E+", NULL } },
{ "WD90C1x", 	{\
		  "WD/Paradise 90C10", "WD/Paradise 90C11", NULL } },
{ "28800", 		{\
		  "ATI 28800-2", "ATI 28800-4", "ATI 28800-5", \
		  "ATI 28800-A", "ATI 28800-C", NULL } },
{ "T8900",		{\
		  "Trident 8900B", "Trident 8900C", "Trident 8900CL", NULL } },
/*  "MISCVGA",	
 "ETW32",		
 "STDVGA" */
{ "QVISION", 	{\
		  "Compaq Advanced VGA", \
		  "Compaq QVision 1024", "Compaq QVision 1280", NULL}  },
{ "WD90C3x",	{\
		  "WD/Paradise 90C30", "WD/Paradise 90C31", NULL } },
{ "WD90C11",	{ "WD/Paradise 90C11", NULL } },
{ "T8900C",	 	{ "Trident 8900C", NULL } },
{ NULL, 		{ NULL } }
};

