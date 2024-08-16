/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:common.h	1.2.1.4"

/*
 *	Copyright (c) 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */

#include <stdio.h>
#include <sys/stat.h>
/*
 * These are appended to either /usr/X or $XWINHOME
 */
#define CONFIGFILE "/defaults/Xwinconfig"
#define VENDORINFO "/lib/display/VendorInfo"
#define DISPLAYDIR "/lib/display/"
#define GRAPHICAL 1
#define NON_GRAPHICAL 0
#define UNIQUE_FILE_LEN	25

struct _graphical_state {
	char *vendor;
	char *monitor;
	char *vcfgfile;
	char *model;
	int xres;
	int yres;
	int depth;
} gstate, *gptr ;

/* MISC NOTES:
 *	/usr/X/lib/display/VendorInfo : this file has the info about
 *	vendors and all the necessary data
 */

/*
 * to store each vendors data
 */
typedef struct _vdata {
	char *vendor;		/* Vendor's name */
	char *model;		/* model name */
	char *chipset;		/* type of chip set */
	char *class;		/* class, ie: Local bus, super VGA or plain */
	char *class_lib;	/* the core drawing library for this class */
	char *vendor_lib;	/* vendor's specific library; this needs the
				   class_lib;ex: et4k_256.so 
				   needs libvga256.so.1 */
	char *configfile;	/* has data about supported modes */
	char *device;		/* device used by the display card */
	char *preinstall_cmd;	/* executes this command before anything, ie:
				   "setvideomode" executes this command as the
				   first step in it's main */
	char *postinstall_cmd;	/* executes this command at the end */
	char *test_cmd;		/* this will be executed to test the mode */
	char *info2vendorlib;	/* his will be executed to test the mode */
	char *description;	/* brief description of this class module */
	char **priv;
	char *vendor_widgetInfo; /* used only for graphical gsetvideo 
				 for formatted line for list widget info */

} vdata;

/*
 * board data for each mode supported
 */
typedef struct _mdata {
	char	*entry;
	int	xmax;
	int	ymax;
	char	*monitor;
	int	depth;
	char	*visual;
	char	*info;
	char	*vfreq;
} mdata;

/*
 * data for Xwinconfig file
 */
struct selection {
	vdata	**vendors;
	int	vendornum;
	mdata	**minfo;
	int	modenum;
};


#ifndef MALLOC

/* #define OUTFILE 1 */
#ifdef OUTFILE
char *
MALLOC (size)
int size;
{
	char *p;
	
	p = (char *)malloc(size);
	fprintf (tmp_fp, "MALLOC: %x\n", p);
	return (p);
}

FREE (ptr)
char *ptr;
{
	fprintf (tmp_fp, "FREE : %x\n", ptr);
	free (ptr);
}
#else
#define MALLOC malloc
#define FREE free
#endif
#endif

typedef struct {
	char		*chipname;	/* Chipset vendor/class name 	*/
	char		*co_processor;	/* Chipset vendor/class name 	*/
	int		memory;
	char		*ramdac;
} BoardInfoRec;

