/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)dd:dd.c	1.18.3.8"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/dd/dd.c,v 1.1 91/02/28 16:49:23 ccs Exp $"
/*
**	convert and copy
*/

#include	<stdio.h>
#include	<signal.h>
#include	<sys/param.h>
#include	<sys/types.h>
#include	<sys/sysmacros.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<string.h>
#include	<errno.h>
#include	<fcntl.h>
#include	<limits.h>      /* LONG_MAX */
#include	<unistd.h>      /* ftruncate() */
#include	<ctype.h>       /* toupper(), tolower() */

#define	BIG	LONG_MAX
#define BSIZE	512

/* Option parameters */

#define COPY		0	/* file copy, preserve input block size */
#define	REBLOCK		1	/* file copy, change block size */
#define	LCREBLOCK	2	/* file copy, convert to lower case */
#define	UCREBLOCK	3	/* file copy, convert to upper case */
#define NBASCII		4	/* file copy, convert from EBCDIC to ASCII */
#define LCNBASCII	5	/* file copy, EBCDIC to lower case ASCII */
#define UCNBASCII	6	/* file copy, EBCDIC to upper case ASCII */
#define NBEBCDIC	7	/* file copy, convert from ASCII to EBCDIC */
#define LCNBEBCDIC	8	/* file copy, ASCII to lower case EBCDIC */
#define UCNBEBCDIC	9	/* file copy, ASCII to upper case EBCDIC */
#define NBIBM		10	/* file copy, convert from ASCII to IBM */
#define LCNBIBM		11	/* file copy, ASCII to lower case IBM */
#define UCNBIBM		12	/* file copy, ASCII to upper case IBM */
#define	UNBLOCK		13	/* convert blocked ASCII to ASCII */
#define	LCUNBLOCK	14	/* convert blocked ASCII to lower case ASCII */
#define	UCUNBLOCK	15	/* convert blocked ASCII to upper case ASCII */
#define	ASCII		16	/* convert blocked EBCDIC to ASCII */
#define	LCASCII		17	/* convert blocked EBCDIC to lower case ASCII */
#define	UCASCII		18	/* convert blocked EBCDIC to upper case ASCII */
#define	BLOCK		19	/* convert ASCII to blocked ASCII */
#define	LCBLOCK		20	/* convert ASCII to lower case blocked ASCII */
#define	UCBLOCK		21	/* convert ASCII to upper case blocked ASCII */
#define	EBCDIC		22	/* convert ASCII to blocked EBCDIC */
#define	LCEBCDIC	23	/* convert ASCII to lower case blocked EBCDIC */
#define	UCEBCDIC	24	/* convert ASCII to upper case blocked EBCDIC */
#define	IBM		25	/* convert ASCII to blocked IBM */
#define	LCIBM		26	/* convert ASCII to lower case blocked IBM */
#define	UCIBM		27	/* convert ASCII to upper case blocked IBM */
#define	LCASE		01	/* flag - convert to lower case */
#define	UCASE		02	/* flag - convert to upper case */
#define	SWAB		04	/* flag - swap bytes before conversion */
#define NERR		010	/* flag - proceed on input errors */
#define SYNC		020	/* flag - pad short input blocks with nulls */
#define NOTRUNC		040     /* flag - do not truncate the output file */
#define BADLIMIT	5	/* give up if no progress after BADLIMIT tries */

/* Global references */

/* Local routine declarations */

int		match();
void		term();
unsigned int	number();
unsigned char	*flsh();
void		stats();
void		alloc_bufs();

/* Local data definitions */

static unsigned ibs;	/* input buffer size */
static unsigned obs;	/* output buffer size */
static unsigned bs;	/* buffer size, overrules ibs and obs */
static unsigned cbs;	/* conversion buffer size, used for block conversions */
static unsigned ibc;	/* number of bytes still in the input buffer */
static unsigned obc;	/* number of bytes in the output buffer */
static unsigned cbc;	/* number of bytes in the conversion buffer */

static int	ibf;	/* input file descriptor */
static int	obf;	/* output file descriptor */
static int	cflag;	/* conversion option flags */
static int	skipf;	/* if skipf == 1, skip rest of input line */
static int	nifr;	/* count of full input records */
static int	nipr;	/* count of partial input records */
static int	nofr;	/* count of full output records */
static int	nopr;	/* count of partial output records */
static int	ntrunc;	/* count of truncated input lines */
static int	nbad;	/* count of bad records since last good one */
static int	files;	/* number of input files to concatenate (tape only) */
static int	skip;	/* number of input records to skip */
static int	iseekn;	/* number of input records to seek past */
static int	oseekn;	/* number of output records to seek past */
static int	count;	/* number of input records to copy (0 = all) */

static char		*string;	/* command arg pointer */
static char		*ifile;		/* input file name pointer */
static char		*ofile;		/* output file name pointer */
static unsigned char	*ibuf;		/* input buffer pointer */
static unsigned char	*obuf;		/* output buffer pointer */

/* error code for dd : 
 * 12: invalid argrument
 * 1:  write-protected/permission
 * 2:  cannot open
 * 3:  skip failed
 * 4:  errors from input file
 * 5:  errors from output file
 * 6:  no memory
 */


/* This is an EBCDIC to ASCII conversion table	*/
/* from a proposed BTL standard April 16, 1979	*/

static unsigned char etoa [] =
{
	0000,0001,0002,0003,0234,0011,0206,0177,
	0227,0215,0216,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0235,0205,0010,0207,
	0030,0031,0222,0217,0034,0035,0036,0037,
	0200,0201,0202,0203,0204,0012,0027,0033,
	0210,0211,0212,0213,0214,0005,0006,0007,
	0220,0221,0026,0223,0224,0225,0226,0004,
	0230,0231,0232,0233,0024,0025,0236,0032,
	0040,0240,0241,0242,0243,0244,0245,0246,
	0247,0250,0325,0056,0074,0050,0053,0174,
	0046,0251,0252,0253,0254,0255,0256,0257,
	0260,0261,0041,0044,0052,0051,0073,0176,
	0055,0057,0262,0263,0264,0265,0266,0267,
	0270,0271,0313,0054,0045,0137,0076,0077,
	0272,0273,0274,0275,0276,0277,0300,0301,
	0302,0140,0072,0043,0100,0047,0075,0042,
	0303,0141,0142,0143,0144,0145,0146,0147,
	0150,0151,0304,0305,0306,0307,0310,0311,
	0312,0152,0153,0154,0155,0156,0157,0160,
	0161,0162,0136,0314,0315,0316,0317,0320,
	0321,0345,0163,0164,0165,0166,0167,0170,
	0171,0172,0322,0323,0324,0133,0326,0327,
	0330,0331,0332,0333,0334,0335,0336,0337,
	0340,0341,0342,0343,0344,0135,0346,0347,
	0173,0101,0102,0103,0104,0105,0106,0107,
	0110,0111,0350,0351,0352,0353,0354,0355,
	0175,0112,0113,0114,0115,0116,0117,0120,
	0121,0122,0356,0357,0360,0361,0362,0363,
	0134,0237,0123,0124,0125,0126,0127,0130,
	0131,0132,0364,0365,0366,0367,0370,0371,
	0060,0061,0062,0063,0064,0065,0066,0067,
	0070,0071,0372,0373,0374,0375,0376,0377,
};

/* This is an ASCII to EBCDIC conversion table	*/
/* from a proposed BTL standard April 16, 1979	*/

static unsigned char atoe [] =
{
	0000,0001,0002,0003,0067,0055,0056,0057,
	0026,0005,0045,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0074,0075,0062,0046,
	0030,0031,0077,0047,0034,0035,0036,0037,
	0100,0132,0177,0173,0133,0154,0120,0175,
	0115,0135,0134,0116,0153,0140,0113,0141,
	0360,0361,0362,0363,0364,0365,0366,0367,
	0370,0371,0172,0136,0114,0176,0156,0157,
	0174,0301,0302,0303,0304,0305,0306,0307,
	0310,0311,0321,0322,0323,0324,0325,0326,
	0327,0330,0331,0342,0343,0344,0345,0346,
	0347,0350,0351,0255,0340,0275,0232,0155,
	0171,0201,0202,0203,0204,0205,0206,0207,
	0210,0211,0221,0222,0223,0224,0225,0226,
	0227,0230,0231,0242,0243,0244,0245,0246,
	0247,0250,0251,0300,0117,0320,0137,0007,
	0040,0041,0042,0043,0044,0025,0006,0027,
	0050,0051,0052,0053,0054,0011,0012,0033,
	0060,0061,0032,0063,0064,0065,0066,0010,
	0070,0071,0072,0073,0004,0024,0076,0341,
	0101,0102,0103,0104,0105,0106,0107,0110,
	0111,0121,0122,0123,0124,0125,0126,0127,
	0130,0131,0142,0143,0144,0145,0146,0147,
	0150,0151,0160,0161,0162,0163,0164,0165,
	0166,0167,0170,0200,0212,0213,0214,0215,
	0216,0217,0220,0152,0233,0234,0235,0236,
	0237,0240,0252,0253,0254,0112,0256,0257,
	0260,0261,0262,0263,0264,0265,0266,0267,
	0270,0271,0272,0273,0274,0241,0276,0277,
	0312,0313,0314,0315,0316,0317,0332,0333,
	0334,0335,0336,0337,0352,0353,0354,0355,
	0356,0357,0372,0373,0374,0375,0376,0377,
};

/* Table for ASCII to IBM (alternate EBCDIC) code conversion	*/

static unsigned char atoibm[] =
{
	0000,0001,0002,0003,0067,0055,0056,0057,
	0026,0005,0045,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0074,0075,0062,0046,
	0030,0031,0077,0047,0034,0035,0036,0037,
	0100,0132,0177,0173,0133,0154,0120,0175,
	0115,0135,0134,0116,0153,0140,0113,0141,
	0360,0361,0362,0363,0364,0365,0366,0367,
	0370,0371,0172,0136,0114,0176,0156,0157,
	0174,0301,0302,0303,0304,0305,0306,0307,
	0310,0311,0321,0322,0323,0324,0325,0326,
	0327,0330,0331,0342,0343,0344,0345,0346,
	0347,0350,0351,0255,0340,0275,0137,0155,
	0171,0201,0202,0203,0204,0205,0206,0207,
	0210,0211,0221,0222,0223,0224,0225,0226,
	0227,0230,0231,0242,0243,0244,0245,0246,
	0247,0250,0251,0300,0117,0320,0241,0007,
	0040,0041,0042,0043,0044,0025,0006,0027,
	0050,0051,0052,0053,0054,0011,0012,0033,
	0060,0061,0032,0063,0064,0065,0066,0010,
	0070,0071,0072,0073,0004,0024,0076,0341,
	0101,0102,0103,0104,0105,0106,0107,0110,
	0111,0121,0122,0123,0124,0125,0126,0127,
	0130,0131,0142,0143,0144,0145,0146,0147,
	0150,0151,0160,0161,0162,0163,0164,0165,
	0166,0167,0170,0200,0212,0213,0214,0215,
	0216,0217,0220,0232,0233,0234,0235,0236,
	0237,0240,0252,0253,0254,0255,0256,0257,
	0260,0261,0262,0263,0264,0265,0266,0267,
	0270,0271,0272,0273,0274,0275,0276,0277,
	0312,0313,0314,0315,0316,0317,0332,0333,
	0334,0335,0336,0337,0352,0353,0354,0355,
	0356,0357,0372,0373,0374,0375,0376,0377,
};

/* Table for conversion of ASCII to lower case ASCII	*/

static unsigned char utol[] =
{
	0000,0001,0002,0003,0004,0005,0006,0007,
	0010,0011,0012,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0024,0025,0026,0027,
	0030,0031,0032,0033,0034,0035,0036,0037,
	0040,0041,0042,0043,0044,0045,0046,0047,
	0050,0051,0052,0053,0054,0055,0056,0057,
	0060,0061,0062,0063,0064,0065,0066,0067,
	0070,0071,0072,0073,0074,0075,0076,0077,
	0100,0141,0142,0143,0144,0145,0146,0147,
	0150,0151,0152,0153,0154,0155,0156,0157,
	0160,0161,0162,0163,0164,0165,0166,0167,
	0170,0171,0172,0133,0134,0135,0136,0137,
	0140,0141,0142,0143,0144,0145,0146,0147,
	0150,0151,0152,0153,0154,0155,0156,0157,
	0160,0161,0162,0163,0164,0165,0166,0167,
	0170,0171,0172,0173,0174,0175,0176,0177,
	0200,0201,0202,0203,0204,0205,0206,0207,
	0210,0211,0212,0213,0214,0215,0216,0217,
	0220,0221,0222,0223,0224,0225,0226,0227,
	0230,0231,0232,0233,0234,0235,0236,0237,
	0240,0241,0242,0243,0244,0245,0246,0247,
	0250,0251,0252,0253,0254,0255,0256,0257,
	0260,0261,0262,0263,0264,0265,0266,0267,
	0270,0271,0272,0273,0274,0275,0276,0277,
	0300,0301,0302,0303,0304,0305,0306,0307,
	0310,0311,0312,0313,0314,0315,0316,0317,
	0320,0321,0322,0323,0324,0325,0326,0327,
	0330,0331,0332,0333,0334,0335,0336,0337,
	0340,0341,0342,0343,0344,0345,0346,0347,
	0350,0351,0352,0353,0354,0355,0356,0357,
	0360,0361,0362,0363,0364,0365,0366,0367,
	0370,0371,0372,0373,0374,0375,0376,0377,
};

/* Table for conversion of ASCII to upper case ASCII	*/

static unsigned char ltou[] =
{
	0000,0001,0002,0003,0004,0005,0006,0007,
	0010,0011,0012,0013,0014,0015,0016,0017,
	0020,0021,0022,0023,0024,0025,0026,0027,
	0030,0031,0032,0033,0034,0035,0036,0037,
	0040,0041,0042,0043,0044,0045,0046,0047,
	0050,0051,0052,0053,0054,0055,0056,0057,
	0060,0061,0062,0063,0064,0065,0066,0067,
	0070,0071,0072,0073,0074,0075,0076,0077,
	0100,0101,0102,0103,0104,0105,0106,0107,
	0110,0111,0112,0113,0114,0115,0116,0117,
	0120,0121,0122,0123,0124,0125,0126,0127,
	0130,0131,0132,0133,0134,0135,0136,0137,
	0140,0101,0102,0103,0104,0105,0106,0107,
	0110,0111,0112,0113,0114,0115,0116,0117,
	0120,0121,0122,0123,0124,0125,0126,0127,
	0130,0131,0132,0173,0174,0175,0176,0177,
	0200,0201,0202,0203,0204,0205,0206,0207,
	0210,0211,0212,0213,0214,0215,0216,0217,
	0220,0221,0222,0223,0224,0225,0226,0227,
	0230,0231,0232,0233,0234,0235,0236,0237,
	0240,0241,0242,0243,0244,0245,0246,0247,
	0250,0251,0252,0253,0254,0255,0256,0257,
	0260,0261,0262,0263,0264,0265,0266,0267,
	0270,0271,0272,0273,0274,0275,0276,0277,
	0300,0301,0302,0303,0304,0305,0306,0307,
	0310,0311,0312,0313,0314,0315,0316,0317,
	0320,0321,0322,0323,0324,0325,0326,0327,
	0330,0331,0332,0333,0334,0335,0336,0337,
	0340,0341,0342,0343,0344,0345,0346,0347,
	0350,0351,0352,0353,0354,0355,0356,0357,
	0360,0361,0362,0363,0364,0365,0366,0367,
	0370,0371,0372,0373,0374,0375,0376,0377,
};

static char out_seekerr[] =
	":154:Output seek error: %s\n";
static char out_readerr[] =
	":1155:Read error during seek: %s\n";
static char out_truncerr[] =
	":1156:Output truncate error: %s\n";
static char block_err[] =
	":1157:'block' and 'unblock' are mutually exclusive\n";
static char lucase_err[] =
	":1158:'lcase' and 'ucase' are mutually exclusive\n";
static char out_fstaterr[] =
	":1159:Output fstat() failed: %s\n";
static char trunc1blk[] =
	":1160:1 truncated block\n";
static char truncnblks[] =
	":1161:%u truncated blocks\n";

main(argc, argv)
int argc;
char **argv;
{
	register unsigned char *ip, *op;/* input and output buffer pointers */
	register int c;			/* character counter */
	register int ic;		/* input character */
	register int conv;		/* conversion option code */
	off_t oseekbytes = (off_t)0;    /* # of output bytes to seek */
	
	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:dd");

	/* Set option defaults */

	ibs = BSIZE;
	obs = BSIZE;
	files = 1;
	conv = COPY;

	/* Parse command options */

	for (c = 1; c < argc; c++)
	{
		string = argv[c];
		if (match("ibs="))
		{
			ibs = number();
			continue;
		}
		if (match("obs="))
		{
			obs = number();
			continue;
		}
		if (match("cbs="))
		{
			cbs = number();
			continue;
		}
		if (match("bs="))
		{
			bs = number();
			continue;
		}
		if (match("if="))
		{
			ifile = string;
			continue;
		}
		if (match("of="))
		{
			ofile = string;
			continue;
		}

		if (match("skip="))
		{
			skip = number();
			continue;
		}
		if (match("iseek="))
		{
			iseekn = number();
			continue;
		}
		if (match("oseek="))
		{
			oseekn = number();
			continue;
		}
		if (match("seek="))		/* retained for compatibility */
		{
			oseekn = number();
			continue;
		}
		if (match("count="))
		{
			count = number();
			continue;
		}
		if (match("files="))
		{
			files = number();
			continue;
		}
		if (match("conv="))
		{
			for (;;)
			{
				if (match(","))
				{
					continue;
				}
				if (*string == '\0')
				{
					break;
				}
				if (match("block"))
				{
					if (conv == UNBLOCK)
                                        {
                                                pfmt(stderr, MM_ERROR,
                                                        block_err);
                                                exit(12);
                                        }
					conv = BLOCK;
					continue;
				}
				if (match("unblock"))
				{
					if (conv == BLOCK)
                                        {
                                                pfmt(stderr, MM_ERROR,
                                                        block_err);
                                                exit(12);
                                        }
					conv = UNBLOCK;
					continue;
				}

				if (match("ebcdic"))
				{
					conv = EBCDIC;
					continue;
				}
				if (match("ibm"))
				{
					conv = IBM;
					continue;
				}
				if (match("ascii"))
				{
					conv = ASCII;
					continue;
				}
				if (match("lcase"))
				{
					cflag |= LCASE;
					continue;
				}
				if (match("ucase"))
				{
					cflag |= UCASE;
					continue;
				}
				if (match("swab"))
				{
					cflag |= SWAB;
					continue;
				}
				if (match("noerror"))
				{
					cflag |= NERR;
					continue;
				}
				if (match("sync"))
				{
					cflag |= SYNC;
					continue;
				}
				if (match("notrunc"))
				{
					cflag |= NOTRUNC;
					continue;
				}
				goto badarg;
			}
			continue;
		}
		if (match("-?"))
		{
			fprintf(stderr, "Usage:\ndd [option=value] ...\n");
		} else {
			badarg:
			pfmt(stderr, MM_ERROR, ":145:Bad argument: \"%s\"\n", string);
		}
		exit(12);
	}

	/* Perform consistency checks on options, decode strange conventions */

	if (bs)
	{
		ibs = obs = bs;
	}
	if ((ibs == 0) || (obs == 0))
	{
		pfmt(stderr, MM_ERROR, ":146:Buffer sizes cannot be zero\n");
		exit(12);
	}
	if (conv == COPY)
	{
		if (cbs != 0)
		{
			pfmt(stderr, MM_ERROR,
				":147:cbs must be zero if no block conversion requested\n");
			exit(12);
		}
		if ((bs == 0) || (cflag&(LCASE|UCASE)))
		{
			conv = REBLOCK;
		}
	}
	if (cbs == 0)
	{
		switch (conv)
		{
		case BLOCK:
		case UNBLOCK:
			conv = REBLOCK;
			break;

		case ASCII:
			conv = NBASCII;
			break;

		case EBCDIC:
			conv = NBEBCDIC;
			break;

		case IBM:
			conv = NBIBM;
			break;
		}
	}
	
	if (cflag & UCASE)
	{
		if (cflag & LCASE)
		{
			pfmt(stderr, MM_ERROR, lucase_err);
			exit(12);
		}
		else
		{
			for (c = 0; c < sizeof ltou; c++)
			{
				ltou[c] = toupper(c);
			}
		}
	}
	else if (cflag & LCASE)
	{
		if (cflag & UCASE)
		{
			pfmt(stderr, MM_ERROR, lucase_err);
			exit(12);
		}
		else
		{
			for (c = 0; c < sizeof utol; c++)
			{
				utol[c] = tolower(c);
			}
		}
	}

	/* Expand options into lower and upper case versions if necessary */

	switch (conv)
	{
	case REBLOCK:
		if (cflag&LCASE)
		{
			conv = LCREBLOCK;
		}
		else if (cflag&UCASE)
		{
			conv = UCREBLOCK;
		}
		break;

	case UNBLOCK:
		if (cflag&LCASE)
		{
			conv = LCUNBLOCK;
		}
		else if (cflag&UCASE)
		{
			conv = UCUNBLOCK;
		}
		break;

	case BLOCK:
		if (cflag&LCASE)
		{
			conv = LCBLOCK;
		}
		else if (cflag&UCASE)
		{
			conv = UCBLOCK;
		}
		break;

	case ASCII:
		if (cflag&LCASE)
		{
			conv = LCASCII;
		}
		else if (cflag&UCASE)
		{
			conv = UCASCII;
		}
		break;

	case NBASCII:
		if (cflag&LCASE)
		{
			conv = LCNBASCII;
		}
		else if (cflag&UCASE)
		{
			conv = UCNBASCII;
		}
		break;

	case EBCDIC:
		if (cflag&LCASE)
		{
			conv = LCEBCDIC;
		}
		else if (cflag&UCASE)
		{
			conv = UCEBCDIC;
		}
		break;

	case NBEBCDIC:
		if (cflag&LCASE)
		{
			conv = LCNBEBCDIC;
		}
		else if (cflag&UCASE)
		{
			conv = UCNBEBCDIC;
		}
		break;

	case IBM:
		if (cflag&LCASE)
		{
			conv = LCIBM;
		}
		else if (cflag&UCASE)
		{
			conv = UCIBM;
		}
		break;

	case NBIBM:
		if (cflag&LCASE)
		{
			conv = LCNBIBM;
		}
		else if (cflag&UCASE)
		{
			conv = UCNBIBM;
		}
		break;
	}

	/* Open the input file, or duplicate standard input */

	ibf = -1;
	if (ifile)
	{
		ibf = open(ifile, 0);
	}
#ifndef STANDALONE
	else
	{
		ifile = "";
		ibf = dup(0);
	}
#endif
	if (ibf == -1)
	{
		pfmt(stderr, MM_ERROR,
			":4:Cannot open %s: %s\n", ifile,
			strerror(errno));
		exit(2);
	}

	/* Open the output file, or duplicate standard output */

	obf = -1;
	if (ofile)
	{
		int     oflag;

                oflag =  O_CREAT;
                oflag |= (oseekn ? O_RDWR : O_WRONLY);
		obf = open(ofile, oflag, 0666);
	}
#ifndef STANDALONE
	else
	{
		ofile = "";
		obf = dup(1);
	}
#endif
	if (obf == -1)
	{
		pfmt(stderr, MM_ERROR,
			":148:Cannot create %s: Write-protected disk or %s\n", ofile,
			strerror(errno));
		exit(1);
	}

	alloc_bufs(conv);

	/* Enable a statistics message on SIGINT */

#ifndef STANDALONE
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
	{
		(void)signal(SIGINT, term);
	}
#endif

	/* Skip input blocks */

	while (skip)
	{
		ibc = read(ibf, (char *)ibuf, ibs);
		if (ibc == (unsigned)-1)
		{
			if (++nbad > BADLIMIT)
			{
				pfmt(stderr, MM_ERROR, ":150:Skip failed: %s\n",
					strerror(errno));
				exit(3);
			}
			else
			{
				pfmt(stderr, MM_ERROR,
					":151:Read error during skip: %s\n",
					strerror(errno));
			}
		}
		else
		{
			if (ibc == 0)
			{
				pfmt(stderr, MM_ERROR,
					":152:Cannot skip past end-of-file: %s\n",
					strerror(errno));
				exit(3);
			}
			else
			{
				nbad = 0;
			}
		}
		skip--;
	}

	/* Seek past input blocks */

	if (iseekn && lseek(ibf, ((off_t) iseekn) * ((off_t) ibs), 1) == -1)
	{
		pfmt(stderr, MM_ERROR, ":153:Input seek error: %s\n",
			strerror(errno));
		exit(4);
	}

	/* Seek past output blocks */

	if (oseekn)
	{
		oseekbytes = ((off_t) oseekn) * ((off_t) obs);

		if (lseek(obf, oseekbytes, SEEK_CUR) == -1)
		{
			while (oseekn > 0)
			{
				obc = (unsigned)read(obf,
					(char *)obuf, obs);
				if (obc == (unsigned)-1)
				{
					if (++nbad > BADLIMIT)
					{
						pfmt(stderr, MM_ERROR,
							out_seekerr,
							strerror(
							errno));
						exit(5);
					}
					else
					{
						pfmt(stderr, MM_ERROR,
							out_readerr,
							strerror(
							errno));
					}
				}
				else
				{
					if (obc < obs)
					{
						break;
					}
					else
					{
						nbad = 0;
					}
				}
				oseekn--;
			}

			(void)memset(obuf, (int)'\0', (size_t)obs);

			if ((obc < obs) && (obc != 0))
			{
				obc = obs - obc;
				if ((unsigned)write(obf, (char *)obuf,
					obc) != obc)
				{
					pfmt(stderr, MM_ERROR,
						out_seekerr,
						strerror(errno));
					exit(5);
				}
				oseekn--;
			}

			while (oseekn > 0)
			{
				if ((unsigned)write(obf, (char *)obuf,
					obs) != obs)
				{
					pfmt(stderr, MM_ERROR,
						out_seekerr,
						strerror(errno));
					exit(5);
				}
				oseekn--;
			}
		}
	}
	else
	{
		oseekbytes = (off_t)0;
	}

	if (!(cflag & NOTRUNC))
	{
		struct stat	statbuf;

		if (fstat(obf, &statbuf) == -1)
		{
			pfmt(stderr, MM_ERROR, out_fstaterr,
				strerror(errno));
			exit(5);
		}
		else if (S_ISREG(statbuf.st_mode))
		{
			if (ftruncate(obf, oseekbytes) == -1)
			{
				pfmt(stderr, MM_ERROR, out_truncerr,
					strerror(errno));
				exit(5);
			}
		}
	}


	/* Initialize all buffer pointers */

	skipf = 0;	/* not skipping an input line */
	ibc = 0;	/* no input characters yet */
	obc = 0;	/* no output characters yet */
	cbc = 0;	/* the conversion buffer is empty */
	op = obuf;	/* point to the output buffer */

	/* Read and convert input blocks until end of file(s) */

	for (;;)
	{
		if ((count == 0) || (nifr+nipr < count))
		{
			/* If proceed on error is enabled, zero the input buffer */

			if (cflag&NERR)
			{
				ip = ibuf + ibs;
				c = ibs;
				if (c & 1)		/* if the size is odd, */
				{
					*--ip = 0;	/* clear the odd byte */
				}
				if (c >>= 1)		/* divide by two */
				{
					do {		/* clear two at a time */
						*--ip = 0;
						*--ip = 0;
					} while (--c);
				}
			}

			/* Read the next input block */

			ibc = read(ibf, (char *)ibuf, ibs);

			/* Process input errors */

			if (ibc == (unsigned)-1)
			{
				pfmt(stderr, MM_ERROR,
					":155:Read error: %s\n", strerror(errno));
				if (   ((cflag&NERR) == 0)
				    || (++nbad > BADLIMIT) )
				{
					while (obc)
					{
						(void)flsh();
					}
					term(4);
				}
				else
				{
					stats();
					ibc = ibs;	/* assume a full block */
				}
			}
			else
			{
				if (ibc > ibs) {
					pfmt(stderr, MM_ERROR, ":160:dd: block size mismatch\n");
					exit(12);
				}
				nbad = 0;
			}
		}

		/* Record count satisfied, simulate end of file */

		else
		{
			ibc = 0;
			files = 1;
		}

		/* Process end of file */

		if (ibc == 0)
		{
			switch (conv)
			{
			case UNBLOCK:
			case LCUNBLOCK:
			case UCUNBLOCK:
			case ASCII:
			case LCASCII:
			case UCASCII:

				/* Trim trailing blanks from the last line */

				if ((c = cbc) != 0)
				{
					do {
						if ((*--op) != ' ')
						{
							op++;
							break;
						}
					} while (--c);
					*op++ = '\n';
					obc -= cbc - c - 1;
					cbc = 0;

					/* Flush the output buffer if full */

					while (obc >= obs)
					{
						op = flsh();
					}
				}
				break;

			case BLOCK:
			case LCBLOCK:
			case UCBLOCK:
			case EBCDIC:
			case LCEBCDIC:
			case UCEBCDIC:
			case IBM:
			case LCIBM:
			case UCIBM:

				/* Pad trailing blanks if the last line is short */

				if (cbc)
				{
					obc += c = cbs - cbc;
					cbc = 0;
					if (c > 0)
					{
						/* Use the right kind of blank */

						switch (conv)
						{
						case BLOCK:
						case LCBLOCK:
						case UCBLOCK:
							ic = ' ';
							break;

						case EBCDIC:
						case LCEBCDIC:
						case UCEBCDIC:
							ic = atoe[' '];
							break;

						case IBM:
						case LCIBM:
						case UCIBM:
							ic = atoibm[' '];
							break;
						}

						/* Pad with trailing blanks */

						do {
							*op++ = ic;
						} while (--c);
					}
				}

				/* Flush the output buffer if full */

				while (obc >= obs)
				{
					op = flsh();
				}
				break;
			}

			/* If no more files to read, flush the output buffer */

			if (--files <= 0)
			{
				(void)flsh();
				term(0);	/* successful exit */
			}
			else
			{
				close(ibf);
				ibf = open(ifile, 0);
				continue;	/* read the next file */
			}
		}

		/* Normal read, check for special cases */

		else if (ibc == ibs)
		{
			nifr++;		/* count another full input record */
		}
		else
		{
			nipr++;		/* count a partial input record */

			/* If `sync' enabled, pad nulls */

			if ((cflag&SYNC) && ((cflag&NERR) == 0))
			{
				c = ibs - ibc;
				ip = ibuf + ibs;
				do {
					*--ip = 0;
				} while (--c);
				ibc = ibs;
			}
		}

		/* Swap the bytes in the input buffer if necessary */

		if (cflag&SWAB)
		{
			ip = ibuf;
			if (ibc & 1)		/* if the byte count is odd, */
			{
				ip[ibc] = 0;	/* make it even, pad with zero */
			}
			c = (ibc + 1) >> 1;	/* compute the pair count */
			do {
				ic = *ip++;
				ip[-1] = *ip;
				*ip++ = ic;
			} while (--c);		/* do two bytes at a time */
		}

		/* Select the appropriate conversion loop */

		ip = ibuf;
		switch (conv)
		{

		/* Simple copy: no conversion, preserve the input block size */

		case COPY:
			obc = ibc;
			(void)flsh();
			break;

		/* Simple copy: pack all output into equal sized blocks */

		case REBLOCK:
		case LCREBLOCK:
		case UCREBLOCK:
		case NBASCII:
		case LCNBASCII:
		case UCNBASCII:
		case NBEBCDIC:
		case LCNBEBCDIC:
		case UCNBEBCDIC:
		case NBIBM:
		case LCNBIBM:
		case UCNBIBM:
			while ((c = ibc) != 0)
			{
				if (c > (obs - obc))
				{
					c = obs - obc;
				}
				ibc -= c;
				obc += c;
				switch (conv)
				{
				case REBLOCK:
					do {
						*op++ = *ip++;
					} while (--c);
					break;

				case LCREBLOCK:
					do {
						*op++ = utol[*ip++];
					} while (--c);
					break;

				case UCREBLOCK:
					do {
						*op++ = ltou[*ip++];
					} while (--c);
					break;

				case NBASCII:
					do {
						*op++ = etoa[*ip++];
					} while (--c);
					break;

				case LCNBASCII:
					do {
						*op++ = utol[etoa[*ip++]];
					} while (--c);
					break;

				case UCNBASCII:
					do {
						*op++ = ltou[etoa[*ip++]];
					} while (--c);
					break;

				case NBEBCDIC:
					do {
						*op++ = atoe[*ip++];
					} while (--c);
					break;

				case LCNBEBCDIC:
					do {
						*op++ = atoe[utol[*ip++]];
					} while (--c);
					break;

				case UCNBEBCDIC:
					do {
						*op++ = atoe[ltou[*ip++]];
					} while (--c);
					break;

				case NBIBM:
					do {
						*op++ = atoibm[*ip++];
					} while (--c);
					break;

				case LCNBIBM:
					do {
						*op++ = atoibm[utol[*ip++]];
					} while (--c);
					break;

				case UCNBIBM:
					do {
						*op++ = atoibm[ltou[*ip++]];
					} while (--c);
					break;
				}
				if (obc >= obs)
				{
					op = flsh();
				}
			}
			break;

		/* Convert from blocked records to lines terminated by newline */

		case UNBLOCK:
		case LCUNBLOCK:
		case UCUNBLOCK:
		case ASCII:
		case LCASCII:
		case UCASCII:
			while ((c = ibc) != 0)
			{
				if (c > (cbs - cbc))	/* if more than one record, */
				{
					c = cbs - cbc;	/* only copy one record */
				}
				ibc -= c;
				cbc += c;
				obc += c;
				switch (conv)
				{
				case UNBLOCK:
					do {
						*op++ = *ip++;
					} while (--c);
					break;

				case LCUNBLOCK:
					do {
						*op++ = utol[*ip++];
					} while (--c);
					break;

				case UCUNBLOCK:
					do {
						*op++ = ltou[*ip++];
					} while (--c);
					break;

				case ASCII:
					do {
						*op++ = etoa[*ip++];
					} while (--c);
					break;

				case LCASCII:
					do {
						*op++ = utol[etoa[*ip++]];
					} while (--c);
					break;

				case UCASCII:
					do {
						*op++ = ltou[etoa[*ip++]];
					} while (--c);
					break;
				}

				/* Trim trailing blanks if the line is full */

				if (cbc == cbs)
				{
					c = cbs;	/* `do - while' is usually */
					do {		/* faster than `for' */
						if ((*--op) != ' ')
						{
							op++;
							break;
						}
					} while (--c);
					*op++ = '\n';
					obc -= cbs - c - 1;
					cbc = 0;

					/* Flush the output buffer if full */

					while (obc >= obs)
					{
						op = flsh();
					}
				}
			}
			break;

		/* Convert to blocked records */

		case BLOCK:
		case LCBLOCK:
		case UCBLOCK:
		case EBCDIC:
		case LCEBCDIC:
		case UCEBCDIC:
		case IBM:
		case LCIBM:
		case UCIBM:
			while ((c = ibc) != 0)
			{
				int nlflag = 0;

				/* We may have to skip to the end of a long line */

				if (skipf)
				{
					do {
						if ((ic = *ip++) == '\n')
						{
							skipf = 0;
							c--;
							break;
						}
					} while (--c);
					if ((ibc = c) == 0)
					{
						continue;	/* read another block */
					}
				}

				/* If anything left, copy until newline */

				if (c > (cbs - cbc + 1))
				{
					c = cbs - cbc + 1;
				}
				ibc -= c;
				cbc += c;
				obc += c;

				switch (conv)
				{
				case BLOCK:
					do {
						if ((ic = *ip++) != '\n')
						{
							*op++ = ic;
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case LCBLOCK:
					do {
						if ((ic = *ip++) != '\n')
						{
							*op++ = utol[ic];
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case UCBLOCK:
					do {
						if ((ic = *ip++) != '\n')
						{
							*op++ = ltou[ic];
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case EBCDIC:
					do {
						if ((ic = *ip++) != '\n')
						{
							*op++ = atoe[ic];
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case LCEBCDIC:
					do {
						if ((ic = *ip++) != '\n')
						{
							*op++ = atoe[utol[ic]];
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case UCEBCDIC:
					do {
						if ((ic = *ip++) != '\n')
						{
							*op++ = atoe[ltou[ic]];
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case IBM:
					do {
						if ((ic = *ip++) != '\n')
						{
							*op++ = atoibm[ic];
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case LCIBM:
					do {
						if ((ic = *ip++) != '\n')
						{
							*op++ = atoibm[utol[ic]];
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;

				case UCIBM:
					do {
						if ((ic = *ip++) != '\n')
						{
							*op++ = atoibm[ltou[ic]];
						}
						else
						{
							nlflag = 1;
							break;
						}
					} while (--c);
					break;
				}

				/* If newline found, update all the counters and */
 				/* pointers, pad with trailing blanks if necessary */

				if (nlflag)
				{
					ibc += c - 1;
					obc += cbs - cbc;
					c += cbs - cbc;
					cbc = 0;
					if (c > 0)
					{
						/* Use the right kind of blank */

						switch (conv)
						{
						case BLOCK:
						case LCBLOCK:
						case UCBLOCK:
							ic = ' ';
							break;

						case EBCDIC:
						case LCEBCDIC:
						case UCEBCDIC:
							ic = atoe[' '];
							break;

						case IBM:
						case LCIBM:
						case UCIBM:
							ic = atoibm[' '];
							break;
						}

						/* Pad with trailing blanks */

						do {
							*op++ = ic;
						} while (--c);
					}
				}

				/* If not end of line, this line may be too long */

				else if (cbc > cbs)
				{
					skipf = 1;	/* note skip in progress */
					obc--;
					op--;
					cbc = 0;
					ntrunc++;	/* count another long line */
				}

				/* Flush the output buffer if full */

				while (obc >= obs)
				{
					op = flsh();
				}
			}
			break;
		}
	}
	
}

/* match ************************************************************** */
/*									*/
/* Compare two text strings for equality				*/
/*									*/
/* Arg:		s - pointer to string to match with a command arg	*/
/* Global arg:	string - pointer to command arg				*/
/*									*/
/* Return:	1 if match, 0 if no match				*/
/*		If match, also reset `string' to point to the text	*/
/*		that follows the matching text.				*/
/*									*/
/* ********************************************************************	*/

int match(s)
char *s;
{
	register char *cs;

	cs = string;
	while (*cs++ == *s)
	{
		if (*s++ == '\0')
		{
			goto true;
		}
	}
	if (*s != '\0')
	{
		return(0);
	}

true:
	cs--;
	string = cs;
	return(1);
}

/* number ************************************************************* */
/*									*/
/* Convert a numeric arg to binary					*/
/*									*/
/* Global arg:	string - pointer to command arg				*/
/*									*/
/* Valid forms:	123 | 123k | 123w | 123b | 123*123 | 123x123		*/
/*		plus combinations such as 2b*3kw*4w			*/
/*									*/
/* Return:	converted number					*/
/*									*/
/* ********************************************************************	*/

unsigned int number()
{
	register char *cs;
	char *termp;
	long n;
	long cut = BIG / 10;	/* limit to avoid overflow */

	n = strtol(string, &termp, 10);
	cs = termp;
	for (;;)
	{
		switch (*cs++)
		{

		case 'k':
			n *= 1024;
			continue;

		case 'w':
			n *= 2;
			continue;

		case 'b':
			n *= BSIZE;
			continue;

		case '*':
		case 'x':
			string = cs;
			n *= number();

		/* Fall into exit test, recursion has read rest of string */

		/* End of string, check for a valid number */

		case '\0':
			if ((n > cut) || (n < 0))
			{
				pfmt(stderr, MM_ERROR,
					":156:Argument out of range: \"%lu\"\n",
					n);
				exit(12);
			}
			return(n);

		default:
			pfmt(stderr, MM_ERROR,
				":157:Bad numeric argument: \"%s\"\n", string);
			exit(12);
		}
	} /* never gets here */
}

/* flsh *************************************************************** */
/*									*/
/* Flush the output buffer, move any excess bytes down to the beginning	*/
/*									*/
/* Arg:		none							*/
/* Global args:	obuf, obc, obs, nofr, nopr				*/
/*									*/
/* Return:	Pointer to the first free byte in the output buffer.	*/
/*		Also reset `obc' to account for moved bytes.		*/
/*									*/
/* ********************************************************************	*/

unsigned char *flsh()
{
	register unsigned char *op, *cp;
	register unsigned int bc;
	register unsigned int oc;

	if (obc)			/* don't flush if the buffer is empty */
	{
		if (obc >= obs)
		{
			oc = obs;
			nofr++;		/* count a full output buffer */
		}
		else
		{
			oc = obc;
			nopr++;		/* count a partial output buffer */
		}
		bc = write(obf, (char *)obuf, oc);
		if (bc != oc)
		{
			pfmt(stderr, MM_ERROR, ":1:Write error: %s\n",
				strerror(errno));
			if (obc >= obs)
				nofr--;
			else
				nopr--;
			term(5);
		}
		obc -= oc;
		op = obuf;

		/* If any data in the conversion buffer, move it into the output buffer */

		if (obc)
		{
			cp = obuf + obs;
			bc = obc;
			do {
				*op++ = *cp++;
			} while (--bc);
		}
		return(op);
	}
	return(obuf);
}

/* term *************************************************************** */
/*									*/
/* Write record statistics, then exit					*/
/*									*/
/* Arg:		c - exit status code					*/
/*									*/
/* Return:	no return, calls exit					*/
/*									*/
/* ********************************************************************	*/

void
term(c)
int c;
{
	stats();
	exit(c);
}

/* stats ************************************************************** */
/*									*/
/* Write record statistics onto standard error				*/
/*									*/
/* Args:	none							*/
/* Global args:	nifr, nipr, nofr, nopr, ntrunc				*/
/*									*/
/* Return:	void							*/
/*									*/
/* ********************************************************************	*/

void stats()
{
	pfmt(stderr, MM_NOSTD, ":158:%u+%u records in\n", nifr, nipr);
	pfmt(stderr, MM_NOSTD, ":159:%u+%u records out\n", nofr, nopr);
	if (ntrunc == 1)
		pfmt(stderr, MM_NOSTD, trunc1blk);
	else if (ntrunc > 1)
		pfmt(stderr, MM_NOSTD, truncnblks, ntrunc);
}

void
alloc_bufs(conv)
int	conv;
{
	unsigned long	pagesize = sysconf(_SC_PAGESIZE);
	unsigned long	incr;
	

	/* Expand memory to get an input buffer */

	ibuf = (unsigned char *)malloc(ibs + 10 + (pagesize - 1));
	incr = ((unsigned long) ibuf) % pagesize;
	if (incr != 0)
		ibuf += (pagesize - incr);

	/* If no conversions, the input buffer is the output buffer */

	if (conv == COPY)
	{
		obuf = ibuf;
	}

	/* Expand memory to get an output buffer.  Leave enough room at the
	 * end to convert a logical record when doing block conversions.
	 */

	else
	{
		obuf = (unsigned char *)malloc(obs + cbs + 10 + (pagesize - 1));
		incr = ((unsigned long) obuf) % pagesize;
		if (incr != 0)
			obuf += (pagesize - incr);
	}
	if ((ibuf == (unsigned char *)NULL) || (obuf == (unsigned char *)NULL))
	{
		pfmt(stderr, MM_ERROR, ":149:Not enough memory: %s\n",
			strerror(errno));
		exit(6);
	}
}
