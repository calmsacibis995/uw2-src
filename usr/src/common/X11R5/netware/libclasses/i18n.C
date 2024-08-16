#ident	"@(#)libclass:i18n.C	1.2"
#include "i18n.h"
#include <stdio.h>

extern "C" char *gettxt (char *, char *);

static const char FS_CHR = '\001';

/* GetStr
 *
 * Get an internationalized string.  String id's contain both the filename:id
 * and default string, separated by the FS_CHR character.
 */
char * I18n::GetStr (char *idstr)
{
    	char	*sep, *str;
	char	*p,	buf[BUFSIZ];
	int	i = 0;

    	if ((sep = strchr (idstr, FS_CHR)) != NULL) {

		p = idstr;
		while (*p != FS_CHR)
			buf[i++] = *p++;  
		buf[i] = '\0';
	
    		str = gettxt (buf, (char *)(sep + 1));
    		return (str);
	}
	else
		return NULL;
}	/* End of GetStr () */

