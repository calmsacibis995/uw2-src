#ident	"@(#)unixtsa:common/cmd/unixtsa/tsad/config_file.C	1.3"

/********************************************************
 ****** General useful functions and definitions ********
 ********************************************************/


#include	<stdio.h>
#include	<ctype.h>
#include	<string.h>

#include	"config_file.h"

#define _MAXLINELEN 512

static FILE *ConfigFP = NULL ;
static char ReadPattern[_MAXLINELEN] = { '\0' } ;

void
settsaconfigent(char *configFile)
    {
    ConfigFP = fopen(configFile, "r") ;
    }

void
endtsaent(void)
    {
    if ( ConfigFP != NULL )
	fclose(ConfigFP) ;
    }

struct tsaconfigent *
gettsaconfigent(void)
{
	static char inBuffer[_MAXLINELEN] ;
	static struct tsaconfigent tsastruct ;
	struct tsaconfigent	*rcode = NULL ;
	char *ptr ;
	int	rc ;

	if ( ConfigFP == NULL )
		return(NULL);

	if ( ReadPattern[0] == '\0' )
	    {
	    // first time through initialize the read pattern
	    sprintf(ReadPattern, "%%%ds %%%ds %%%ds %%%ds %%%ds",
		MAXHOSTNAMELEN, MAXPROTOLEN, MAXCODESETLEN, MAXCODESETLEN, MAXCODESETLEN) ;
	    }

	while ( (feof(ConfigFP) == 0) && (rcode == NULL) )
	    {
	    ptr = fgets(inBuffer, _MAXLINELEN, ConfigFP) ;

	    // skip beginning whitespace
	    while ( (rc = isspace((int) *ptr)) != 0 )
		ptr++ ;

	    if ( *ptr != '#' ) // skip line if it is commented out
		{
		// zero out any old data
		tsastruct.hostName[0] = '\0' ;
		tsastruct.protocol[0] = '\0' ;
		tsastruct.tsaCodeset[0] = '\0' ;
		tsastruct.engineCodeset[0] = '\0' ;
		tsastruct.engineLocale[0] = '\0' ;

		rc = sscanf(ptr, ReadPattern,
				tsastruct.hostName,
				tsastruct.protocol,
				tsastruct.tsaCodeset,
				tsastruct.engineCodeset,
				tsastruct.engineLocale) ;
	    
		if ( rc > 1 )
		    rcode = &tsastruct ;
		}
	    }

	return(rcode);
}
