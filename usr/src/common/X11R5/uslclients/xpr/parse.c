/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)xpr:parse.c	1.7"

/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "string.h"
#include "stdio.h"

#include "xpr.h"
#include "xgetopt.h"

extern double		atof();

extern int		atoi(),
			getopt(),
			optind,
			opterr,
			optopt;

extern char		*optarg;

				/* 0 - didn't specify -D.
				 * See `D' in xoptions[] for usages. */
int			use_this_depth = 0;
				/* position 0 - specify data amount;
				 * position 1-16 - the data.
				 * See 'P' in xoptions[] for usages. */
#define ARRAY_SIZE	17
unsigned char *		color_index_array = NULL;
static void		setup_color_array(char *, unsigned char *, int);

static void
usage (int print_QDPB)
{
#define	P	(void) printf ("%s\n",
#define	X	);
P "usage:"								X

	if (print_QDPB) {	/* to get this, use xpr -q */
P "   The following options are valid if `-d postscript' presents:"	X
P "	[-D 2 or 4]		(use gray scale 2 or 4 to generate)"	X
P "				( the postscript output.)"		X
P "	[-P P1,P2,...,Pn]	(use pixel P1, P2..., Pn to generate)"	X
P "				( the postscript output.)"		X
P "				(  . n <= 4 if -D 2, n <= 16 if -D 4)"	X
P "				(  . use `-q u' to find out the index)"	X
P "				(  . valid only if -D presents)"	X
P "	Note that using -D can reduce postscript output size."		X
P "		  The output quality may be increased by specifying -P,"X
P "		  without -P, -D 2 will increase each pixel's RGB by"	X
P "		  65535/3 starting from 0 and -D 4 will use 65535/15."	X
P "	[-q query-type]		(query various info, a, d, i, o, and u)"X
P "				(  a - d + i + u)"			X
P "				(  d - postscript data)"		X
P "				(  i - pixel intensity data)"		X
P "				(  I - use IBM prologue section)"	X
P "				(  o - old method, for comparsion only)"X
P "				(  u - pixel usages)"			X
	return;
	}

P "    xpr [options] [file]"						X
P "	[-o output-file]	(output to output-file (overwrite))"	X
P "	[-a output-file [-n]]	(append to output-file [no page break])"X
P "	[-d device-name]	(prepare output for device-name)"	X
P "	[-h string]		(center string 1/4 inch above image)"	X
P "	[-t string]		(center string 1/4 inch below image)"	X
P "	[-W n-inches]		(maximum width of image is n-inches)"	X
P "	[-H n-inches]		(maximum height of image is n-inches)"	X
P "	[-l]			(rotate 90 degrees left (landscape))"	X
P "	[-p]			(don't rotate (portrait) (default))"	X
P "	[-L n-inches]		(left margin of n-inches, else center)"	X
P "	[-T n-inches]		(top margin of n-inches, else center)"	X
P "	[-s n-pages]		(split output into n-pages)"		X
P "	[-S n-dots]		(scale each pixel to n-dots)"		X
P "	[-c]			(compact output (default, so obsolete))"X
P "	[-r]			(reverse \"video\")"			X
#if	defined(FOCUS)
P "	[-f location]		(focus in on large images)"		X
#endif
#if	defined(IMPROVE)
P "	[-i]			(improve image quality (runs slower))"	X
#endif
P "	[-C color-list]		(list ribbon colors)"			X
P "	[file]			(get input from file, else stdin)"	X
#undef P
#undef X
}

struct xoptions			xoptions[] = {
	'o', "output",		1,
	'a', "append",		1,
	'n', "noff",		0,
	'd', "device",		1,
	'h', "header",		1,
	't', "trailer",		1,
	'w', "width",		1,
	'W', 0,			1,	/* synonym for -w */
	'H', "height",		1,
	'l', "landscape",	0,
	'p', "portrait",	0,
	'L', "left",		1,
	'T', "top",		1,
	's', "split",		1,
	'S', "scale",		1,
	'c', "compact",		0,
	'r', "rv",		0,
#if	defined(FOCUS)
	'f', "focus",		1,
#endif
#if	defined(IMPROVE)
	'i', "improve",		0,
#endif
	'C', "color",		1,

	/*
	 * The following are here for compatibility with earlier
	 * versions of the xpr program. These were undocumented
	 * options, so did not show up in the requirements.
	 */
	'N', "nosixopt",	0,
	'R', "report",		0,

	/*
	 * The following is an undocumented option for changing
	 * the color mapping scheme. It is here until someone decides
	 * to take it out.
	 */
	'b', "bw",		1,

	/* The following is a new option. When -D depth (depth is 2 or 4)
	 * is specified, it will increase each pixel's RGB by 65535/(depth-1)
	 * starting from 0 to generate the postscript output, if the
	 * specified depth value is greater than or equal to the
	 * depth value in src_image, than the specified depth value
	 * is ignored, the legel depth value is 2 (4 colors) or
	 * 4 (16 colors). e.g., if you have a 256 color (i.e., depth 8),
	 * you can get a depth 4 postscript output file if `-D 4' presents.
	 *
	 * Note that this option is valid only if `-d postscript' presents.
	 *	     You may want to use -P to increase the output quality.
	 */
	'D', "depth",	1,

	/* The following is a new option. When -P is specified,
	 * the given pixels will be used to replace the ones in src_colors.
	 * Input is comma seperated and should have no space in between. e.g.,
	 * -P 3,2,1,15 -D 2 will use pixel index, 3, 2, 1, and 15 to
	 * produce the postscript output. You can specify 16 pixels at most
	 * for `-D 4' and 4 pixels for `-D 2'.
	 *
	 * This is a way to increase output quality when your postscript
	 * printer can't handle large amount of data (i.e., force you
	 * to use -D).
	 *
	 * Note that this option is valid only if -d postscript and -D present.
	 */
	'P', "pixel-list", 1,

	/* The following option is to query various info while generating
	 * the postscript output.
	 * It recognizes the following keyboards/letters -
	 *	u[sage]		- dump out pixel usages (output for -P),
	 *			  `-q o' is ignored if this option presents;
	 *	i[ntensity]	- dump out pixel information;
	 *	I[BM-prologue]	- use prologue section from IBM dealer to
	 *			  work around a VM problem in their earier
	 *			  postscript version. This looks better, maybe
	 *			  we should use it as default, for now, enabled
	 *			  it via `-q I'.
	 *	d[ata]		- dump out the pixel data;
	 *	a[ll]		- d[ata], i[ntensity], and u[sage];
	 *	o[ld-method]	- use old algorithm to generate the ps output,
	 *			  i.e., it will dump out the color array info,
	 *				and setup a proc for settransfer to
	 *				use. The raw data are `pixel' values.
	 *			  Otherwise, it will apply the grey scale to
	 *			  the `pixel' value and then dump them out as
	 *			  the raw data;
	 *	others		- usage(print_QDPB).
	 * Note that this option is valid only if -d postscript presents.
	 */
	'q', "query",		1,

	0,   0,			0
};

static int		isterminfo();

/**
 ** parse_args()
 **/

void	
parse_args (argc, argv, scale, width, height, left, top, device, flags, split, header, trailer, color_list)
	int			argc;
	char			**argv;
	int			*scale;
	double			*width,
				*height,
				*left,
				*top;
	Device			**device;
	int			*flags,
				*split;
	char			**header,
				**trailer,
				**color_list;
{
	register char		*output_filename	= 0,
				*s_device		= DEFAULT_DEVICE;

	int			optlet;
	int			a_option = 0, o_option = 0;


	*flags = 0;
	*split = 1;
	*width = -1;
	*height = -1;
	*top = -1;
	*left = -1;
	*header = 0;
	*trailer = 0;
	
	opterr = 0;
	while ((optlet = xgetopt(argc, argv, xoptions)) != -1)

		switch (optlet) {

		case 'a':
			if (o_option) {
				fprintf ( 
					stderr,
		"xpr: Error: The options -a/-append and -o/-output cannot be used together.\n"
				);
				exit (1);
			}
			a_option = 1;
			output_filename = optarg;
			/* See if file exists, else treat as -o option */
			if (!access(output_filename, 00))
				*flags |= F_APPEND;
			break;

		case 'd':
			s_device = optarg;
			break;

		case 'h':
			if (strlen (optarg))
				*header = optarg;
			break;

		case 'H':
			MyAtof(optarg,height,"height");
			CheckArgBad(*height, "height");
			CheckArgZero(*height, "height");
			break;

		case 'l':
			*flags |= F_LANDSCAPE;
			break;

		case 'L':
			MyAtof(optarg,left,"left");
			CheckArgBad(*left, "left");
			break;

		case 'n':
			*flags |= F_NOFF;
			break;

		case 'N':
			fprintf (
				stderr,
		"xpr: Warning: The -nosixopt option is obsolete.\n"
			);
			break;

		case 'o':
			if (a_option) {
				fprintf ( 
					stderr,
		"xpr: Error: The options -a/-append and -o/-output cannot be used together.\n"
				);
				exit (1);
			}
			o_option = 1;
			output_filename = optarg;
			break;

		case 'p':
			*flags |= F_PORTRAIT;
			break;

		case 'c':
			fprintf (
				stderr,
		"xpr: Warning: The -compact option is obsolete.\n"
			);
			break;

		case 'R':
			*flags |= F_REPORT;
			break;

		case 's':
			MyAtoi(optarg,split,"split");
			CheckArgBad((double) *split, "split");
			CheckArgZero((double) *split, "split");
			break;

		case 'S':
			MyAtoi(optarg,scale,"scale");
			CheckArgBad((double) *scale, "scale");
			CheckArgZero((double) *scale, "scale");
			break;

		case 't':
			if (strlen (optarg))
				*trailer = optarg;
			break;

		case 'T':
			MyAtof(optarg,top,"top");
			CheckArgBad(*top, "top");
			break;

		case 'w':
		case 'W':
			MyAtof(optarg,width,"width");
			CheckArgBad(*width, "width");
			CheckArgZero(*width, "width");
			break;

		case 'r':
			*flags |= F_INVERT;
			break;

#if	defined(FOCUS)
		case 'f':
			break;
#endif

#if	defined(IMPROVE)
		case 'i':
			break;
#endif

		case 'C':
			*color_list = optarg;
			break;

			/*
			 * This option is not supported, and is here
			 * for testing different color to black-and-white
			 * mappings. The argument should be the set of
			 * coefficients used to convert RGB into a single
			 * gray-scale value.
			 */
		case 'b':
			{
				extern double		atod();

#define DSTRTOK(S,D)	atof(strtok(S,D))
				rgbmap.red = DSTRTOK(optarg, ",");
				rgbmap.green = DSTRTOK((char *)0, ",");
				rgbmap.blue = DSTRTOK((char *)0, ",");
			}
			break;

		case 'D':
			if (*optarg == '2')
				use_this_depth = 2;	/* 4 colors */
			else if (*optarg == '4')
				use_this_depth = 4;	/*16 colors */
			break;

		case 'P':
			color_index_array = (unsigned char *)malloc(
							ARRAY_SIZE *
							sizeof(unsigned char));
			setup_color_array(optarg,color_index_array, ARRAY_SIZE);
			break;

		case 'q':
			if (*optarg == 'd')
				*flags |= F_DUMP_PSDATA;
			else if (*optarg == 'i')
				*flags |= F_DUMP_PIXINTENSITY;
			else if (*optarg == 'u')
				*flags |= F_DUMP_PIXUSAGES;
			else if (*optarg == 'a')
				*flags |= F_DUMP_PSDATA |
					  F_DUMP_PIXINTENSITY |
					  F_DUMP_PIXUSAGES;
			else if (*optarg == 'o')
				*flags |= F_USESETTRANSFER;
			else if (*optarg == 'I')
				*flags |= F_USEIBMPROLOG;
			else {
				usage(1);
				exit(0);
			}
			break;

		case '?':
			/*
			 * "optopt" is an undocumented feature of
			 * "getopt()". It appears to contain the option
			 * letter where things went wrong. If it's an
			 * option we recognize, the problem must be a
			 * missing argument.
			 */
			if (optopt == '?' || optopt == 'q') {
				usage(optopt == '?' ? 0 : 1);
				exit(0);
			} else {
				struct xoptions		*po;


				for (po = xoptions; po->letter; po++)
					if (po->letter == optopt)
						break;
				if (po->letter)
					fprintf (
						stderr,
		"xpr: Error: The option -%c requires an argument.\n",
						optopt
					);
				else
					fprintf (
						stderr,
		"xpr: Error: The option -%c is not recognized.\n",
						optopt
					);
			}
			exit (1);
			/*NOTREACHED*/

		}

	if (*flags & F_DUMP_PIXUSAGES) {

		*flags &= ~F_USESETTRANSFER;	/* disable -q o if -q u */
	}

	if (*flags & F_NOFF && !(*flags & F_APPEND))
		fprintf (
			stderr,
"xpr: Warning: The -n option is ignored unless the -a option is used.\n"
		);

	for (*device = device_list; (*device)->name; (*device)++)
		if (
			STREQU((*device)->name, s_device)
		     || (*device)->terminfo && isterminfo(s_device)
		)
			break;
	if (!(*device)->name) {
		fprintf (
			stderr,
			"xpr: Device \"%s\" not supported.\n",
			s_device
		);
		exit(1);
	}

	/*
	 * Other routines want to know if the output device
	 * is a postscript printer, thus the following:
	 */
	if (
		STREQU(s_device, "ps")
	     || STREQU(s_device, "lw")
	     || STREQU(s_device, "laserwriter")
	)
		s_device = "postscript";
	if ((*device)->terminfo)
		(*device)->name = strdup(s_device);

	if (optind < argc) {
		input_filename = argv[optind];
		if (!freopen(input_filename, "r", stdin)) {
			fprintf (
				stderr,
				"xpr: Error opening \"%s\" for input (%s).\n",
				argv[optind],
				PERROR (errno)
			);
			exit (1);
		}
	}

	if (output_filename) {
		if (!freopen(
			output_filename,
			(*flags & F_APPEND? "a+" : "w"),
			stdout
		)) {
			fprintf (
				stderr,
				"xpr: Error opening \"%s\" for output (%s).\n", 
				output_filename,
				PERROR (errno)
			);
			exit (1);
		}
		if (*flags & F_APPEND)
			fseek (stdout, 0L, 2);
		/*
		 * The device dependent routine(s) are responsible for
		 * backing up over a terminating ``formfeed'' in an
		 * appended file (if -noff), because a ``formfeed'' is
		 * not the same for all devices.
		 */
	}

	return;
}

/**
 ** isterminfo() - SEE IF NAME IS VALID AND USABLE TERMINFO ENTRY
 **/

static int		isterminfo (name)
	char			*name;
{
/*
 * MORE: This probably belongs in the terminfo library
 */
	/*
	 * "ps" is already used for a different device.
	 */
	if (STREQU(name, "ps"))
		name = "postscript";

	return (tidbit(name, "porder", (short *)0) != -1);
}

#define	HUGE	99999

CheckArgZero(value,name)
double	value;
char	*name;
{
	if ( value == 0) {
		fprintf (stderr,"xpr:  Error:  Value for %s out of range\n",name);
		exit (1);
	}
	return (0);
}

CheckArgBad(value,name)
double	value;
char	*name;
{
	if ( (value < 0) || (value > HUGE) ) {
		fprintf (stderr,"xpr:  Error:  Value for %s out of range\n",name);
		exit (1);
	}
	return (0);
}

MyAtof(token,variable,name)
char	*token;
double	*variable;
char	*name;
{
	if (*token != '.' && *token != '-' && ( *token < '0' || *token > '9')) {
		fprintf (stderr,"xpr:  Error:  Value for %s is non-numeric\n",name);
		exit(1);
	}
	*variable = atof(token);

}

MyAtoi(token,variable,name)
char	*token;
int	*variable;
char	*name;
{
	if (*token != '-' && (*token < '0' || *token > '9')) {
		fprintf (stderr,"xpr:  Error:  Value for %s must be an integer\n",name);
		exit(1);
	}
	*variable = atoi(token);
}


static void
setup_color_array(char * token, unsigned char * this_array, int this_size)
{
	char *	ptr;
	char *	prev;
	char *	keep_this;

	keep_this = ptr = prev = strdup(token);

#define IDX	this_array[0]

	while (*ptr) {

		if (*ptr < '0' || *ptr > '9') {	/* separator */

			*ptr = 0;
			if (*prev) {
				this_array[++IDX] = atoi(prev);

				if (IDX == this_size)
					break;
			}
			prev = ptr + 1;
		}
		*ptr++;
	}
	if (IDX != this_size && *prev) {
		this_array[++IDX] = atoi(prev);
	}

	free(keep_this);
#undef IDX
}
