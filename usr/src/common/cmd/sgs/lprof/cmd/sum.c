/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lprof:cmd/sum.c	1.8.3.4"
#include <stdio.h>

#include "symint.h"
#include <time.h>
#include <pfmt.h>
#include <sys/stat.h>

#include "retcode.h"
#include "funcdata.h"
#include "glob.h"
#include "env.h"
#include "coredefs.h"

#include "sgs.h"

extern size_t 	demangle();
extern void 	*_lprof_Malloc();
extern 		int C_flag;

#ifdef __STDC__
#define DATESIZE        60
#endif

#define NAMELEN		100	/* for use by C++ name demangler */

CAsumhdr(type,src,obj)
short type;
char *src;
char *obj;
{

    struct stat stat_ptr;

#ifdef __STDC__
    char   buf[DATESIZE];
#else
    extern char  *ctime( );
#endif

    if((src == NULL) || (obj == NULL))
	return(0);  /* no processor or no oject file name specified */
	
    if (stat(src, &stat_ptr) != 0)
	return(0);  /* cannot get status of file */

    pfmt(stdout,MM_NOSTD,":92:Coverage Data Source: %s\n",src);
#ifdef __STDC__

    if ( strftime(buf,DATESIZE,"%a %b %d %H:%M:%S %Y", 
                  localtime((const time_t *) &(stat_ptr.st_mtime)))   ==  0 ) {
	(void) fprintf(stderr, "%slprof: insufficient space to store date\n", SGS);
	return(0);
       	}
    (void) printf("Date of Coverage Data Source: %s\n", buf );

#else

    (void) pfmt(stdout,MM_NOSTD,":1281:Date of Coverage Data Source: %.24s\n", ctime(&(stat_ptr.st_mtime)));

#endif
    pfmt(stdout,MM_NOSTD,":1282:Object: %s\n\n",obj);

    if (type == SUM) {
	pfmt(stdout,MM_NOSTD,":1283:percent   lines   total\tfunction\n");
	pfmt(stdout,MM_NOSTD,":1284:covered  covered  lines\tname\n\n", "name");
    }

    return(1);   /* success */

}

static long total_lines = 0;
static long total_cov = 0;

CAsumrept(func)
struct caFUNC_DATA *func;
{

	struct caDATA_BLK *blk;
	float lst_percent, percent;
	long lines_in_func, count;	/* covered line counter */
	char *buf;
	size_t len;
	int i;

	count = 0;
	lines_in_func = 0;
	lst_percent = 0;
	percent = 0;
	i = 0;

	blk = func->data;
	while (blk != NULL) {
	    for(i=0;i < (unsigned int)blk->entry_cnt;i++)  {
		lines_in_func++;
		if(blk->stats->data[i].count > 0)
		    count++;
	    }
	    blk = blk->next_blk;
	}
	
	if (C_flag) {
		buf =  (char *) _lprof_Malloc(NAMELEN, 1);

		if (((len = demangle(func->func_name, buf, NAMELEN)) 
		     != (size_t) -1)  &&  (len > NAMELEN)) {
			free(buf);
			buf =  (char *) _lprof_Malloc(len, 1);
			len = demangle(func->func_name, buf, len);
		}
		if (len != (size_t)-1) {
			free(func->func_name);
			func->func_name = buf; 
		}
	}
		
	
	if(func->data != NULL)  {
	   if(count == 0)
	  	percent = 0;
	   else  {
		percent = (float)count/(float)lines_in_func;
		lst_percent = percent * 100;
	   }
	}
	else {
	   pfmt(stderr,MM_ERROR,":1285:No lines in function %s\n",func->func_name);
	   return(-1);   /* failure */
	}

	total_cov = total_cov + count;
	total_lines = total_lines + lines_in_func;


       pfmt(stdout,MM_NOSTD,":1286:%6.1f%8ld%8ld\t%s\n",
	    lst_percent, count, lines_in_func, func->func_name);
       return(1);  /* success */


}	/* end of main */

CAsum_tot()
{
    if (total_lines != 0) {
	pfmt(stdout,MM_NOSTD, ":1287:\n%6.1f%8d%8d\tTOTAL\n",
	    100.0*(float)total_cov/(float)total_lines, total_cov, total_lines);
    }
}

