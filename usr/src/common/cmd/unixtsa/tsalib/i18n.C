#ident	"@(#)unixtsa:common/cmd/unixtsa/tsalib/i18n.C	1.1"
/***
 *
 *  name	i18n.C - codeset conversion routines for unixtsa
 *		@(#)unixtsa:common/cmd/unixtsa/tsalib/i18n.C	1.1	6/7/94
 *
 ***/

#include	<string.h>
#include	<i18n.h>

/* globals */
iconv_t	TsaToEngine = (iconv_t) -1, EngineToTsa = (iconv_t) -1 ;

/* local prototypes */
char	*ConvertString( iconv_t Conv, char *Source, size_t *NewSize ) ;

/***
 *
 *  name	ConvertInitialize - set up globals for codeset conversions
 *		arguments are two character set names, must be exact
 *
 *  notes	need to add more intelligence if set names not found
 *
 *  returns	0 if codeset conversion is set up, non-zero if no
 *		such conversion exists.
 *
 ***/

int
ConvertInitialize( char *TsaSet, char *EngineSet )
    {
    int 	rcode ;

    // iconv_open will return (iconv_t) -1 if conversion is not valid
    if ( TsaSet != NULL && EngineSet != NULL && *TsaSet != '\0' && *EngineSet != '\0' )
	{
		if((TsaToEngine = iconv_open(TsaSet, EngineSet)) 
					== (iconv_t)-1){
			EngineToTsa = (iconv_t) -1 ;
			rcode = -1 ;
		}
		else if ((EngineToTsa = iconv_open(EngineSet, TsaSet))
					== (iconv_t)-1){
			TsaToEngine = (iconv_t) -1 ;
			rcode = -1 ;
		}
		else {
			rcode = 0 ;
		}
	}
    else
	{
	TsaToEngine = (iconv_t) -1 ;
	EngineToTsa = (iconv_t) -1 ;
	rcode = -1 ;
	}

    return(rcode) ;
    }

/***
 *
 *  name	ConvertString - convert a string to a different codeset
 *
 ***/

char *
ConvertString( iconv_t Conv, char *Source, size_t *NewSize )
    {
    static char		ConversionBuffer[CONVERSION_SIZE] ;
    char		*rvalue ;
    char		*outptr, *inptr ;
    size_t		outremainder, inremainder ;

    if ( Conv == (iconv_t) -1 )
	{
	rvalue = Source ;
	}
    else
	{
	inremainder = strlen(Source) ;
	inptr = Source ;
	outptr = ConversionBuffer ;
	outremainder = CONVERSION_SIZE ;

	while ( inremainder > 0 && outremainder > 0 )
	    {
	    if ( (iconv(Conv, &inptr, &inremainder, &outptr, &outremainder)) == (size_t) -1 )
		{
		/* string could not be converted */
		break ;
		}
	    }

	/* make certain of null termination */
	if ( outremainder == 0 )
	    outptr-- ;

	*outptr = '\0' ;
	rvalue = ConversionBuffer ;
	}

    if ( NewSize != NULL )
	*NewSize = strlen(rvalue) ;

    return(rvalue) ;
    }

/***
 *
 *  name	ConvertTsaToEngine
 *
 ***/

char *
ConvertTsaToEngine( char *InputString, size_t *NewSize )
    {
    return( ConvertString(TsaToEngine, InputString, NewSize) ) ;
    }

/***
 *
 *  name	ConvertEngineToTsa
 *
 ***/

char *
ConvertEngineToTsa( char *InputString, size_t *NewSize )
    {
    return( ConvertString(EngineToTsa, InputString, NewSize) ) ;
    }
