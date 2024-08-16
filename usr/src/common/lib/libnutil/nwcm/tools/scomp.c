/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
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

#ident	"@(#)libnwutil:common/lib/libnutil/nwcm/tools/scomp.c	1.9"
#ident	"$Id: scomp.c,v 1.8.2.1 1994/10/19 18:24:21 vtag Exp $"
/*
 * Copyright 1992 Unpublished Work of Novell, Inc.
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include <ctype.h>

#include "sc_parser.h"
#include "sc_tune.h"


char	*filename;
int	lineno = 0;

extern char *optarg;
extern int optind;
extern int yyparse(void);

char	*libName = "";
char	*domainRevStr = NULL;
char	*domainFile = NULL;
char	*domainNum = "0";
char	*extra_include = NULL;
char	*outfilepath = NULL;
char	**filelist;

/* forward declarations */
static void	WriteSchema(void);
int 		yyerror(char *message);
int 		yywrap(void);

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

main(int argc, char *argv[])
{
	int	c;

	while ((c = getopt(argc, argv, "o:i:r:f:d:l:")) != -1) {
		switch (c) {
		case 'i':
			extra_include = optarg;
			break;
		case 'f':
			domainFile = optarg;
			break;
		case 'r':
			domainRevStr = optarg;
			break;
		case 'd':
			domainNum = optarg;
			break;
		case 'l':
			libName = optarg;
			break;
		case 'o':
			if (outfilepath) {
				fprintf(stderr,
				    "output file may only be specified once\n");
				return(1);
			}
			outfilepath = optarg;
			break;
		default:
			return(1);
		}
	}

	filelist = &argv[optind];
	
	if (yywrap()) {
		fprintf(stderr, "no input file\n");
		return(1);
	}

	yyparse();

	WriteSchema();

	return(0);
}

#define ERROR_THRESHOLD	10

int
yyerror(char *message)
{
	static int	error_count = 0;
	int		r;

	r = fprintf(stderr, "%s #%d: %s\n", filename, lineno, message);
	if (++error_count > ERROR_THRESHOLD) {
		(void) fprintf(stderr, "%s #%d: too many errors, giving up!\n",
		    filename, lineno);
		exit(1);
	}

	return(r);
}

int
yywrap(void)
{
	extern FILE *	yyin;

	if ((filename = *filelist++) == NULL)
		return(1);
	if ((yyin = fopen(filename, "r")) == NULL)
		return(1);
	lineno = 1;
	return(0);
}

static struct cp_s	ConfigurationParameters[MAX_PARAM];
static struct cp_s	*ConfigurationParameterPtr = ConfigurationParameters;
static struct cp_s	*ConfigurationParametersEnd =
			&ConfigurationParameters[MAX_PARAM];

static unsigned long	IntegerParameterValues[MAX_INT_PARAM];
static int		IntegerParameterIndex = 0;

static struct ipvd_s	IntegerValidationData[MAX_INT_PARAM];
static int		IntegerValidationIndex = 0;

static int		BooleanParameterValues[MAX_BOOL_PARAM];
static int		BooleanParameterIndex = 0;

static char		*StringParameterValues[MAX_STR_PARAM];
static int		StringParameterIndex = 0;

static char		*IdentLines[MAX_IDENT];
static char		**IdentLinePtr = IdentLines;
char		**IdentLinesEnd = &IdentLines[MAX_IDENT];

struct er_s {
	char	*funcname;
	char	*argtype;
};

static int 			StringValidationCount = 0;
static int 			IntegerValidationCount = 0;
static int 			BooleanValidationCount = 0;
static struct er_s	ExternalReferences[MAX_EXTREF];
static struct er_s	*ExternalReferencePtr = ExternalReferences;
static struct er_s	*ExternalReferencesEnd =
			&ExternalReferences[MAX_EXTREF];

static struct cp_s *
get_cp(char *name, enum NWCP type, char *folder, char *description, char *helpString)
{
	struct cp_s	*cpp;
	static int	rel_order = 0;

	if (ConfigurationParameterPtr >= ConfigurationParametersEnd) {
		yyerror("too many parameters");
		return(NULL);
	}

	for (cpp = ConfigurationParameters; cpp < ConfigurationParameterPtr;
	    cpp++) {
		if (strcmpi(name, cpp->name) == 0) {
			yyerror("duplicate parameter name");
			return(NULL);
		}
	}

	memset((void *)ConfigurationParameterPtr, 0, sizeof(struct cp_s));
	ConfigurationParameterPtr->name = name;
	ConfigurationParameterPtr->type = type;
	ConfigurationParameterPtr->folder = folder;
	ConfigurationParameterPtr->rel_order = rel_order++;
	ConfigurationParameterPtr->description = description;
	ConfigurationParameterPtr->helpString = helpString;

	return(ConfigurationParameterPtr++);
}

static int
get_ii(void)
{
	if (IntegerParameterIndex >= MAX_INT_PARAM)
		yyerror("too many integer parameters -- output corrupt!");

	return(IntegerParameterIndex++);
}

static struct ipvd_s *
get_ipvd(void)
{
	struct ipvd_s	*ipvdp;

	if (IntegerValidationIndex >= MAX_INT_PARAM)
		yyerror("too many integer parameters -- output corrupt!");

	ipvdp = &IntegerValidationData[IntegerValidationIndex];

	memset((void *)ipvdp, 0, sizeof(struct ipvd_s));
	ipvdp->index = IntegerValidationIndex++;

	return(ipvdp);
}

static int
get_bi(void)
{
	if (BooleanParameterIndex >= MAX_BOOL_PARAM)
		yyerror("too many boolean parameters -- output corrupt!");

	return(BooleanParameterIndex++);
}

static int
get_si(void)
{
	if (StringParameterIndex >= MAX_STR_PARAM)
		yyerror("too many string parameters -- output corrupt!");

	return(StringParameterIndex++);
}

static char **
get_il(void)
{
	if (IdentLinePtr >= IdentLinesEnd) {
		yyerror("too many ident lines");
		return(NULL);
	}

	return(IdentLinePtr++);
}

static int
cpcmp( const void *s1, const void *s2)
{
	return strcmpi(((struct cp_s *)s1)->name, ((struct cp_s *)s2)->name);
}

static void
stuff_extern(char *funcname, char *argtype)
{
	if (ExternalReferencePtr >= ExternalReferencesEnd) {
		yyerror("too many external references");
		return;
	}

	ExternalReferencePtr->funcname = funcname;
	ExternalReferencePtr->argtype = argtype;

	ExternalReferencePtr++;
}

static void
WriteSchema(void)
{
	FILE		*outfile;
	int		i, j;
	char		*s;
	struct cp_s	*cpp;
	struct er_s	*erp;

	if ((outfile = fopen(outfilepath, "w")) == NULL)
		return;

	/*
	 * Prologue Section
	 */
	fprintf(outfile, "/*\n * DO NOT EDIT THIS FILE\n *\n * This file was machine generated.  Do not make changes here.\n */\n\n#include <schemadef.h>\n\n#ifndef NULL\n#define NULL\t0\n#endif\n\n");
	if(extra_include)
		fprintf(outfile, "#include \"%s\"\n", extra_include);
	fprintf(outfile, "#include \"nwmsg.h\"\n\n");

	/*
	** Provide information about the package.
	*/
	fprintf(outfile, "char\t\t*libName = \"%s\";\n", libName);
	fprintf(outfile, "char\t\t*domainFile = %s;\n", domainFile);
	fprintf(outfile, "char\t\t*domainRevStr = %s;\n", domainRevStr);
	fprintf(outfile, "int \t\tdomain = %s;\n\n", domainNum);
		
	/*
	 * Ident Section
	 */
	IdentLinesEnd = IdentLinePtr;
	IdentLinePtr = IdentLines;
	if (IdentLinePtr < IdentLinesEnd) {
		fprintf(outfile, "static char ident[] = \"%s\";\n",
		    *IdentLinePtr);
		for (IdentLinePtr++, i = 0; IdentLinePtr < IdentLinesEnd;
		    IdentLinePtr++, i++)
			fprintf(outfile, "static char ident%d[] = \"%s\";\n",
			    i, *IdentLinePtr);
		fprintf(outfile, "\n");
	}

	/*
	 * External References Section
	 */
	if(IntegerValidationCount) {
		fprintf(outfile, "char\t\tintValFuncs[%d][NWCM_MAX_STRING_SIZE] = {\n", IntegerValidationCount);
		for (i=0, erp = ExternalReferences; erp < ExternalReferencePtr; erp++)
		{
			if((strcmp(erp->argtype, "unsigned long")) == 0) {
				fprintf(outfile, "\t\"%s\"", erp->funcname);
				if(i < (IntegerValidationCount - 1))
					fprintf(outfile, ",\n");
				else
					fprintf(outfile, "\n");
				i++;
			}
		}
		fprintf(outfile, "};\n\n");
	} else {
		fprintf(outfile, "char\t\tintValFuncs[1][NWCM_MAX_STRING_SIZE] = { \"\" };\n\n");
	}

	if(BooleanValidationCount) {
		fprintf(outfile,
			"char\t\tboolValFuncs[%d][NWCM_MAX_STRING_SIZE] = {\n",
			BooleanValidationCount);
		for (i=0, erp = ExternalReferences; erp < ExternalReferencePtr; erp++)
		{
			if((strcmp(erp->argtype, "int *")) == 0) {
				fprintf(outfile, "\t\"%s\"", erp->funcname);
				if(i < (BooleanValidationCount - 1))
					fprintf(outfile, ",\n");
				else
					fprintf(outfile, "\n");
				i++;
			}
		}
		fprintf(outfile, "};\n\n");
	} else {
		fprintf(outfile, "char\t\tboolValFuncs[1][NWCM_MAX_STRING_SIZE] = { \"\" };\n\n");
	}

	if(StringValidationCount) {
		fprintf(outfile,
			"char\t\tstringValFuncs[%d][NWCM_MAX_STRING_SIZE] = {\n",
			StringValidationCount);
		for (i=0, erp = ExternalReferences; erp < ExternalReferencePtr; erp++)
		{
			if((strcmp(erp->argtype, "char *")) == 0) {
				fprintf(outfile, "\t\"%s\"", erp->funcname);
				if(i < (StringValidationCount - 1))
					fprintf(outfile, ",\n");
				else
					fprintf(outfile, "\n");
				i++;
			}
		}
		fprintf(outfile, "};\n\n");
	} else {
		fprintf(outfile, "char\t\tstringValFuncs[1][NWCM_MAX_STRING_SIZE] = { \"\" };\n\n");
	}

	/*
	 * Integer Data Section
	 */
	if (IntegerParameterIndex) {
		fprintf(outfile, "unsigned long\t\tIntParameterDefaults[%d] = {\n",
		    IntegerParameterIndex);

		for (i = 0, j = IntegerParameterIndex - 1; i < j; i++)
			fprintf(outfile, "\t%lu,\t/* %d */\n", IntegerParameterValues[i],
				i);
		fprintf(outfile, "\t%lu\t/* %d */\n};\n\n", IntegerParameterValues[i],
			i);
	} else {
		fprintf(outfile,
		    "unsigned long\t\tIntParameterDefaults[1] = {\n\t0\n};\n\n");
	}
	fprintf(outfile, "int\t\tIntegerParameterCount = %d;\n\n",
	    IntegerParameterIndex);

	/*
	 * Integer Validation Data Section
	 */
	if (IntegerValidationIndex) {
		fprintf(outfile,
		    "struct ipvd_s\tIntegerValidationData[%d] = {",
		    IntegerValidationIndex);

		for (i = 0; i < IntegerValidationIndex; i++) {
			if (i)
				fprintf(outfile, ",");
			fprintf(outfile, "\n\t{\t/* %d */\n", i);
			if (IntegerValidationData[i].func) {
				fprintf(outfile, "\t\t(ivf_t) %d,\n", i+1);
				j++;
			} else
				fprintf(outfile, "\t\tNULL,\n");
			fprintf(outfile, "\t\t%lu, %lu\n\t}",
			    IntegerValidationData[i].min,
			    IntegerValidationData[i].max);
		}
		fprintf(outfile, "\n};\n");
	} else {
		fprintf(outfile,
		    "struct ipvd_s\t\tIntegerValidationData[1] = {\n\t{\n\t\tNULL,\n\t\t0, 0\n\t}\n};\n");
	}

	/*
	 * Boolean Data Section
	 */
	if (BooleanParameterIndex) {
		fprintf(outfile, "int\t\tBoolParameterDefaults[%d] = {\n",
		    BooleanParameterIndex);

		for (i = 0, j = BooleanParameterIndex - 1; i < j; i++) {
			if (BooleanParameterValues[i]) {
				s = "TRUE";
			} else {
				s = "FALSE";
			}
			fprintf(outfile, "\t%s,\t/* %d */\n", s, i);
		}
		if (BooleanParameterValues[i]) {
			s = "TRUE";
		} else {
			s = "FALSE";
		}
		fprintf(outfile, "\t%s\t/* %d */\n};\n", s, i);
	} else {
		fprintf(outfile,
		    "int\t\tBoolParameterDefaults[1] = {\n\tFALSE\n};\n");
	}
	fprintf(outfile, "int\t\tBooleanParameterCount = %d;\n\n",
	    BooleanParameterIndex);

	/*
	 * String Data Section
	 */
	if (StringParameterIndex) {
		fprintf(outfile,
		    "char\t\tStrParameterDefaults[%d][%s] = {\n",
		    StringParameterIndex, "NWCM_MAX_STRING_SIZE");

		for (i = 0, j = StringParameterIndex - 1; i < j; i++)
			fprintf(outfile, "\t\"%s\",\t/* %d */\n",
			    StringParameterValues[i], i);
		fprintf(outfile, "\t\"%s\"\t/* %d */\n};\n", StringParameterValues[i],
			i);
	} else {
		fprintf(outfile,
		    "char\t\tStrParameterDefaults[1][%s] = {\n\t\"\"\n};\n",
		    "NWCM_MAX_STRING_SIZE");
	}
	fprintf(outfile, "int\t\tStringParameterCount = %d;\n\n",
		StringParameterIndex);

	/*
	 * Configuration Parameter Section
	 */
	fprintf(outfile, "struct cp_s\tConfigParameters[] = {");
	for (i = 0, cpp = ConfigurationParameters;
	    cpp < ConfigurationParameterPtr; cpp++, i++) {
		if (cpp != ConfigurationParameters)
			fprintf(outfile, ",");
		fprintf(outfile,
			"\n\t{\n\t\t\"%s\",\n\t\t%s,\n\t\t%d,\n\t\t%s,\n\t\t%s,\n",
			cpp->name, cpp->folder, cpp->rel_order, cpp->description,
			cpp->helpString);
		switch(cpp->type) {
		case NWCP_INTEGER:
			fprintf(outfile, "\t\tNWCP_INTEGER,\n");
			if (cpp->validation.data) {
				fprintf(outfile,
				    "\t\t&IntegerValidationData[%d],\n",
				    cpp->validation.data->index);
			} else {
				fprintf(outfile, "\t\tNULL,\n");
			}
			switch (cpp->format) {
			case df_octal:
				fprintf(outfile, "\t\tdf_octal,\n");
				break;
			case df_hexadecimal:
				fprintf(outfile, "\t\tdf_hexadecimal,\n");
				break;
			default:
				fprintf(outfile, "\t\tdf_decimal,\n");
			}
			fprintf(outfile, "\t\t&IntParameterDefaults[%d]\n\t}",
			    cpp->def_val);
			break;
		case NWCP_BOOLEAN:
			fprintf(outfile, "\t\tNWCP_BOOLEAN,\n");
			if (cpp->validation.func)
				fprintf(outfile, "\t\t\"%s\",\n",
				    cpp->validation.func);
			else
				fprintf(outfile, "\t\tNULL,\n");
			fprintf(outfile, "\t\tdf_normal,\n");
			fprintf(outfile, "\t\t&BoolParameterDefaults[%d]\n\t}",
			    cpp->def_val);
			break;
		case NWCP_STRING:
			fprintf(outfile, "\t\tNWCP_STRING,\n");
			if (cpp->validation.func)
				fprintf(outfile, "\t\t\"%s\",\n",
				    cpp->validation.func);
			else
				fprintf(outfile, "\t\tNULL,\n");
			if (cpp->format == df_uppercase)
				fprintf(outfile, "\t\tdf_uppercase,\n");
			else
				fprintf(outfile, "\t\tdf_normal,\n");
			fprintf(outfile, "\t\t&StrParameterDefaults[%d],\n\t}",
			    cpp->def_val);
			break;
		default:
			fprintf(outfile, "\t\tNWCP_UDEFINED,\n");
			fprintf(outfile, "\t\tNULL,\n");
			fprintf(outfile, "\t\tNULL,\n");
			fprintf(outfile, "\t\tNULL\n\t}");
			break;
		}
	}
	fprintf(outfile, "\n};\nint\tConfigurationParameterCount = %d;\n", i);
}


void
IntegerParamDef(char *name, char *folder, char *description, char *helpString, enum df_e format, unsigned long def_ival,
    enum VM validateHow, ...)
{
	va_list		vap;
	struct cp_s	*cpp;

	if ((cpp = get_cp(name, NWCP_INTEGER, folder, description, helpString)) == NULL)
		return;

	cpp->def_val = get_ii();
	IntegerParameterValues[cpp->def_val] = def_ival;

	if (validateHow != NO_VALIDATION)
		cpp->validation.data = get_ipvd();
	else
		cpp->validation.data = NULL;

	cpp->format = format;

	if (cpp->validation.data == NULL)
		return;

	va_start(vap, validateHow);

	if (validateHow == FUNCTION_VALIDATION) {
		cpp->validation.data->func = va_arg(vap, char *);
		stuff_extern(cpp->validation.data->func, "unsigned long");
		IntegerValidationCount++;
	} else { /* MIN_MAX_VALIDATION */
		cpp->validation.data->min = va_arg(vap, unsigned long);
		cpp->validation.data->max = va_arg(vap, unsigned long);
	}

	va_end(vap);
}

void
BooleanParamDef(char *name, char *folder, char *description, char *helpString, int def_bval, char *valFunc)
{
	struct cp_s	*cpp;

	if ((cpp = get_cp(name, NWCP_BOOLEAN, folder, description, helpString)) == NULL)
		return;

	cpp->def_val = get_bi();
	BooleanParameterValues[cpp->def_val] = def_bval;
	if ((cpp->validation.func = valFunc) != NULL) {
		stuff_extern(valFunc, "int *");
		BooleanValidationCount++;
	}
}


void
StringParamDef(char *name, char *folder, char *description, char *helpString, enum df_e format, char *def_sval, char *valFunc)
{
	struct cp_s	*cpp;

	if ((cpp = get_cp(name, NWCP_STRING, folder, description, helpString)) == NULL)
		return;

	cpp->format = format;
	cpp->def_val = get_si();
	StringParameterValues[cpp->def_val] = def_sval;
	if ((cpp->validation.func = valFunc) != NULL) {
		stuff_extern(valFunc, "char *");
		StringValidationCount++;
	}
}

void
Ident(char *string)
{
	char	**ilp;

	if ((ilp = get_il()) == NULL)
		return;

	*ilp = string;
}
