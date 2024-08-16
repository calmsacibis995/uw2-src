#ident	"@(#)unixtsa:common/cmd/unixtsa/tsalib/hasmntopt.C	1.1"
/***
 *
 *  name	hasmntopt - parse mount option string
 *
 *  synopsis	char *hasmntopt(struct mnttab	*mnt, char *opt) ;
 *
 ***/

#include	<stdio.h>
#include	<sys/mnttab.h>
#include	<string.h>

char *hasmntopt(struct mnttab *Mnt, char *Opt)

    {
    char	*ptr ;

    ptr = strtok(Mnt->mnt_mntopts, ",") ;
    while ( ptr != NULL )
	{
	if ( strcmp(ptr, Opt) == 0 )
	    return (ptr) ;
	else
	    ptr = strtok((char *)0, ",") ;
	}
    return((char *)0) ;
    }
