/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwcm/nwcm.c	1.13"
#ident	"$Id: nwcm.c,v 1.13.2.1 1994/10/19 18:28:57 vtag Exp $"
/*
 * Copyright 1993 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

/*
 * nwcm -- NetWare Configuration Management command-line utility
 */


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

extern char *		optarg;
extern int			optind;
extern int			opterr;
extern int			optopt;

#include "nwcmmsgs.h"
#include "nwmsg.h"

#define NWCM_SCHEMA_FRIEND
#include "nwcm.h"


static struct cp_s *getparam(char *);
static void			clean(void);
static void			set(char *);
static void			validate(char *);
static void			setorvalidate(char *, int (*)(char *, enum NWCP, void *),
	int, int, int);
static void			reset(char *);
static void			resetparam(struct cp_s *);
static void			view(char *);
static void			viewparam(struct cp_s *);
static void			help(char *);
static void			helpparam(struct cp_s *);
static void			describe(char *);
static void			describeparam(struct cp_s *);
static void			listparam(struct cp_s *);
static void			iterate(char *, void (*)(struct cp_s *));
static void			describe_folder(char *);
static void			form_hints(char *);
static void			menu_hintparam(struct cp_s *);
static void			folder_menu_hints(void);
static void			show_config_file(void);
static void			show_config_dir(void);
static void			show_number_of_folders(void);

static const char *	optstr = "qs:t:r:R:v:V:h:H:d:D:L:f:m:M:cCFnx?";
static const char *	options_requiring_arguments = "strRvVhHdDLfmM";

static int			optq = 0;

static int			nwcmerrcnt = 0;
static int			nwcmopcnt = 0;
#define NWCMMAXERRCNT	255


int
main(int argc, char ** argv)
{
	char *		program;
	int			opt;
	char *tmpDir;
	int 		it_calls=0;


	if (program  = strrchr(argv[0], '/'))
		program++;
	else
		program = argv[0];

	(void) MsgBindDomain(MSG_DOMAIN_NWCM, MSG_DOMAIN_UTIL_FILE, MSG_UTIL_REV_STR);
	(void) MsgBindDomain(MSG_DOMAIN_NWCM_FOLD, MSG_DOMAIN_NWCM_FILE, MSG_NWCM_REV_STR);

	opterr = 0;
	while ((opt = getopt(argc, argv, (char *)optstr)) != EOF) {
		switch (opt) {
		case 'q':
			optq++;
			break;
		case 's':
			set(optarg);
			break;
		case 't':
			validate(optarg);
			break;
		case 'r':
			reset(optarg);
			break;
		case 'R':
			iterate(optarg, resetparam);
			it_calls++;
			break;
		case 'v':
			view(optarg);
			break;
		case 'V':
			iterate(optarg, viewparam);
			it_calls++;
			break;
		case 'h':
			help(optarg);
			break;
		case 'H':
			iterate(optarg, helpparam);
			it_calls++;
			break;
		case 'd':
			describe(optarg);
			break;
		case 'D':
			iterate(optarg, describeparam);
			it_calls++;
			break;
		case 'L':
			iterate(optarg, listparam);
			it_calls++;
			break;
		case 'f':
			describe_folder(optarg);
			break;
		case 'x':
			clean();
			break;
#ifdef DEBUG
		case 'm':
			form_hints(optarg);
			break;
		case 'M':
			iterate(optarg, menu_hintparam);
			it_calls++;
			break;
		case 'n':
			folder_menu_hints();
			break;
#endif
		case 'c':
			show_config_file();
			break;
		case 'C':
			show_config_dir();
			break;
		case 'F':
			show_number_of_folders();
			break;
		case '?':
			nwcmerrcnt++;
			break;
		default:
			if (strchr(options_requiring_arguments, optopt)) {
				(void) fprintf(stderr, MsgGetStr(NWCM_CMD_OPTION_REQ_PARAM),
					optopt);
			} else {
				(void) fprintf(stderr, MsgGetStr(NWCM_CMD_UNKNOWN_OPTION),
					optopt);
			}
			nwcmerrcnt++;
			break;
		}
	}

	while (optind < argc) {
		(void) fprintf(stderr, MsgGetStr(NWCM_CMD_UNKNOWN_ARGUMENT),
			argv[optind++]);
	   nwcmerrcnt++;
	}

	if (nwcmerrcnt || !nwcmopcnt) {
		if(!it_calls) {
			fprintf(stderr, MsgGetStr(NWCM_CMD_USAGE_1), program);
			fprintf(stderr, MsgGetStr(NWCM_CMD_USAGE_2));
			fprintf(stderr, MsgGetStr(NWCM_CMD_USAGE_17));
			fprintf(stderr, MsgGetStr(NWCM_CMD_USAGE_3));
			fprintf(stderr, MsgGetStr(NWCM_CMD_USAGE_4));
			fprintf(stderr, MsgGetStr(NWCM_CMD_USAGE_5));
			fprintf(stderr, MsgGetStr(NWCM_CMD_USAGE_6));
			fprintf(stderr, MsgGetStr(NWCM_CMD_USAGE_7));
			fprintf(stderr, MsgGetStr(NWCM_CMD_USAGE_8));
			fprintf(stderr, MsgGetStr(NWCM_CMD_USAGE_9));
			fprintf(stderr, MsgGetStr(NWCM_CMD_USAGE_10));
			fprintf(stderr, MsgGetStr(NWCM_CMD_USAGE_11));
			fprintf(stderr, MsgGetStr(NWCM_CMD_USAGE_12));
			fprintf(stderr, MsgGetStr(NWCM_CMD_USAGE_13));
			fprintf(stderr, MsgGetStr(NWCM_CMD_USAGE_14));
			fprintf(stderr, MsgGetStr(NWCM_CMD_USAGE_15));
			fprintf(stderr, MsgGetStr(NWCM_CMD_USAGE_16));
			fprintf(stderr, MsgGetStr(NWCM_CMD_USAGE_18));
		}
	}

	if (nwcmerrcnt > NWCMMAXERRCNT)
		nwcmerrcnt = NWCMMAXERRCNT;

	return nwcmerrcnt;
}

/*
 * int strcmpi(s1, s2)
 *
 *	Like strcmp(), but case insensitive.
 */

static int
strcmpi(char * s1, char * s2)
{
	int	r;

	while (*s1 && *s2)
		if ((r = (int) (toupper(*s1++) - toupper(*s2++))) != 0)
			return r;
	return (int) (toupper(*s1) - toupper(*s2));
}


static struct cp_s *
getparam(char * opt)
{
	char *				cp;
	int					ccode;
	static struct cp_s	param;

	while (isspace(*opt))
		opt++;
	for (cp = opt; *cp; )
		cp++;
	cp--;
	while (isspace(*cp))
		*cp-- = 0;

	if (ccode = _LookUpParameter(opt, NWCP_UNDEFINED, &param)) {
		(void) NWCMPerror(ccode, MsgGetStr(NWCM_CMD_LOOKUP_FAILED), opt);
		nwcmerrcnt++;
		return NULL;
	}

	return &param;
}


static void
clean(void)
{
	int ccode;

	if (ccode = NWCMCleanConfig()) {
		fprintf(stderr, MsgGetStr(ccode));
		fprintf(stderr, "\n");
		nwcmerrcnt++;
	}
	exit(nwcmerrcnt);
}


static void
set(char * opt)
{
	setorvalidate(opt, NWCMSetParam, NWCM_CMD_MISSING_VALUE_ON_SET,
		NWCM_CMD_SETPARAM_FAILED, optq);
}


static void
validate(char * opt)
{
	setorvalidate(opt, NWCMValidateParam, NWCM_CMD_MISSING_VALUE_ON_VALIDATE,
		NWCM_CMD_VALIDATEPARAM_FAILED, optq);
}


static void
setorvalidate(char * opt, int (*nwcmfunc)(char *, enum NWCP, void *),
	int valmissingerr, int failederr, int quiet)
{
	char *			vp;
	struct cp_s *	param;
	char *			cp;
	char *			check;
	unsigned long	i;
	int				b;
	int				ccode;

	nwcmopcnt++;

	if ((vp = strchr(opt, '=')) == NULL) {
		(void) fprintf(stderr, MsgGetStr(valmissingerr));
		nwcmerrcnt++;
		return;
	}
	*vp++ = 0;

	if ((param = getparam(opt)) == NULL)
		return;

	while (isspace(*vp))
		vp++;
	for (cp = vp; *cp; )
		cp++;
	cp--;
	while (isspace(*cp))
		*cp-- = 0;

	switch (param->type) {
	case NWCP_INTEGER:
		if(*vp == '-')
			goto invalid;
		i = strtoul(vp, &check, 0);
		if(*check != 0)
			goto invalid;
		if (ccode = nwcmfunc(param->name, NWCP_INTEGER, (void *) &i))
			goto paramopfailed;
		break;
	case NWCP_BOOLEAN:
		b = -1;
		if (strcmpi(vp, "active") == 0) {
			b = 1;
		} else if (strcmpi(vp, "true") == 0) {
			b = 1;
		} else if (strcmpi(vp, "on") == 0) {
			b = 1;
		} else if (strcmpi(vp, "yes") == 0) {
			b = 1;
		} else if (strcmpi(vp, "inactive") == 0) {
			b = 0;
		} else if (strcmpi(vp, "false") == 0) {
			b = 0;
		} else if (strcmpi(vp, "off") == 0) {
			b = 0;
		} else if (strcmpi(vp, "no") == 0) {
			b = 0;
		}
		if (b == -1)
			goto invalid;
		if (ccode = nwcmfunc(param->name, NWCP_BOOLEAN, (void *) &b))
			goto paramopfailed;
		break;
	case NWCP_STRING:
		if (ccode = nwcmfunc(param->name, NWCP_STRING, (void *) vp))
			goto paramopfailed;
		break;
	default:
	invalid:
		ccode = NWCM_INVALID_DATA;
	paramopfailed:
		(void) NWCMPerror(ccode, MsgGetStr(failederr),
			param->name, vp);
		nwcmerrcnt++;
		return;
	}

	if (!quiet)
		view(param->name);
}


static void
reset(char * opt)
{
	resetparam(getparam(opt));
}


static void
resetparam(struct cp_s * param)
{
	int			ccode;

	nwcmopcnt++;

	if (param == NULL)
		return;

	if (ccode = NWCMSetToDefault(param->name)) {
		(void) NWCMPerror(ccode, MsgGetStr(NWCM_CMD_SETTODEFAULT_FAILED),
			param->name);
		nwcmerrcnt++;
		return;
	}

	if (!optq)
		view(param->name);
}


static void
view(char * opt)
{
	viewparam(getparam(opt));
}


static void
viewparam(struct cp_s * param)
{
	nwcmopcnt++;

	if (param == NULL)
		return;

	switch (param->type) {
	case NWCP_INTEGER:
		switch(param->format) {
		case df_octal:
			(void) printf("%s=0%lo\n", param->name,
				*((unsigned long *)param->cur_val));
			break;
		case df_hexadecimal:
			(void) printf("%s=0x%lX\n", param->name,
				*((unsigned long *)param->cur_val));
			break;
		default:
			(void) printf("%s=%lu\n", param->name,
				*((unsigned long *)param->cur_val));
			break;
		}
		break;
	case NWCP_BOOLEAN:
		(void) printf("%s=%s\n", param->name,
			*((int *) param->cur_val) ? "on" : "off");
		break;
	case NWCP_STRING:
		(void) printf("%s=\"%s\"\n", param->name, (char *) param->cur_val);
		break;
	}
}


static void
help(char * opt)
{
	helpparam(getparam(opt));
}


static void
helpparam(struct cp_s * param)
{
	char	*helpString;

	nwcmopcnt++;

	if (param == NULL)
		return;

	if(_GetParamHelpString(param->name, &helpString))
		return;

	(void) printf("%s_help=\"%s\"\n", param->name, helpString);
}


static void
describe(char * opt)
{
	describeparam(getparam(opt));
}


static void
describeparam(struct cp_s * param)
{
	char	*descString;

	nwcmopcnt++;

	if (param == NULL)
		return;

	if(_GetParamDescription(param->name, &descString))
		return;

	(void) printf("%s_desc=\"%s\"\n", param->name, descString);
}


static void
listparam(struct cp_s * param)
{
	nwcmopcnt++;

	if (param == NULL)
		return;

	(void) printf("%s\n", param->name);
}


static void
iterate(char * opt, void (*func)(struct cp_s *))
{
	int		folder = (int) strtol(opt, NULL, 10);
	int		i;

	if (folder) {
		if (folder < 0 || folder >= NWCM_NUM_PARAM_FOLDERS) {
			(void) fprintf(stderr, MsgGetStr(NWCM_CMD_FOLDER_OUT_OF_RANGE),
				folder, NWCM_NUM_PARAM_FOLDERS - 1);
			nwcmerrcnt++;
			return;
		}
		for (i = 0; i < ConfigurationParameterCount; i++)
			if (ConfigurationParameters[i].folder == folder) {
				func(&ConfigurationParameters[i]);
			}
				
	} else {
		for (i = 0; i < ConfigurationParameterCount; i++) {
			func(&ConfigurationParameters[i]);
		}
	}
}


static void
describe_folder(char * opt)
{
	int		folder = (int) strtol(opt, NULL, 10);

	nwcmopcnt++;

	if (folder == 0) {
		for (folder = 1; folder < NWCM_NUM_PARAM_FOLDERS; folder++)
			(void) printf("folder_%d_desc=\"%s\"\n", folder,
				MsgDomainGetStr(MSG_DOMAIN_NWCM_FOLD, folder + 1));
		return;
	}

	if (folder < 0 || folder >= NWCM_NUM_PARAM_FOLDERS) {
		(void) fprintf(stderr, MsgGetStr(NWCM_CMD_FOLDER_OUT_OF_RANGE),
			folder, NWCM_NUM_PARAM_FOLDERS - 1);
		nwcmerrcnt++;
		return;
	}

	(void) printf("folder_%d_desc=\"%s\"\n", folder,
		MsgDomainGetStr(MSG_DOMAIN_NWCM_FOLD, folder + 1));
}


static void
form_hints(char * name)
{
	struct cp_s *	param;
	char *			desc;

	nwcmopcnt++;

	if ((param = getparam(name)) == NULL)
		return;

	if(_GetParamDescription(param->name, &desc))
		return;

	if(param->description == NWCM_PN_DEFAULT)
		desc = param->name;

	(void) printf("name=\"%s\"\n", desc);
	(void) printf("nrow=1\nncol=1\nfrow=1\nfcol=%d\nrows=1\n",
		strlen(desc) + 3);

	switch (param->type) {
	case NWCP_INTEGER:
		printf("columns=12\n");
		switch(param->format) {
		case df_octal:
			(void) printf("value=0%lo\n", *((unsigned long *)param->cur_val));
			break;
		case df_hexadecimal:
			(void) printf("value=0x%lX\n", *((unsigned long *)param->cur_val));
			break;
		default:
			(void) printf("value=%lu\n", *((unsigned long *)param->cur_val));
			break;
		}
		break;
	case NWCP_BOOLEAN:
		printf("columns=8\n");
		(void) printf("value=%s\n", *((int *) param->cur_val) ?
			"on" : "off");
		(void) printf("rmenu={ on off }\n");
		break;
	case NWCP_STRING:
		(void) printf("columns=20\nscroll=true\n");
		(void) printf("value=\"%s\"\n", (char *) param->cur_val);
		break;
	}
}


static void
menu_hintparam(struct cp_s * param)
{
	char *			desc;

	nwcmopcnt++;

	if (param == NULL)
		return;

	if(_GetParamDescription(param->name, &desc))
		return;

	(void) printf("param=%s\n", param->name);
	(void) printf("desc=\"%s\"\n", desc);

	switch (param->type) {
	case NWCP_INTEGER:
		switch(param->format) {
		case df_octal:
			(void) printf("value=\"0%lo\"\n",
				*((unsigned long *)param->cur_val));
			break;
		case df_hexadecimal:
			(void) printf("value=\"0x%lX\"\n",
				*((unsigned long *)param->cur_val));
			break;
		default:
			(void) printf("value=\"%lu\"\n",
				*((unsigned long *)param->cur_val));
			break;
		}
		break;
	case NWCP_BOOLEAN:
		(void) printf("value=\"%s\"\n",
			*((int *) param->cur_val) ? "on" : "off");
		break;
	case NWCP_STRING:
		(void) printf("value=\"%s\"\n", (char *) param->cur_val);
		break;
	}

}


static void
folder_menu_hints(void)
{
	int			folder;

	nwcmopcnt++;

	for (folder = 1; folder < NWCM_NUM_PARAM_FOLDERS; folder++) {
		(void) printf("name=\"%s\"\n", MsgDomainGetStr(MSG_DOMAIN_NWCM_FOLD,
			folder + 1));
		(void) printf("info=%d \"%s\"\n", folder, 
			MsgDomainGetStr(MSG_DOMAIN_NWCM_FOLD, folder + 1));
	}
}


static void
show_config_file(void)
{
	nwcmopcnt++;

	(void) printf("config_file=\"%s\"\n", NWCMGetConfigFilePath());
}


static void
show_config_dir(void)
{
	nwcmopcnt++;

	(void) printf("config_dir=\"%s\"\n", NWCMGetConfigDirPath());
}


static void
show_number_of_folders(void)
{
	nwcmopcnt++;

	(void) printf("folders=%d\n", NWCM_NUM_PARAM_FOLDERS - 1);
}
