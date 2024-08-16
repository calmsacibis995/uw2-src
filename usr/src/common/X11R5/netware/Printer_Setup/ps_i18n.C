#ident	"@(#)prtsetup2:ps_i18n.C	1.3"
/*----------------------------------------------------------------------------
 *	ps_i18n.c
 */

#include <stdio.h>
#include <string.h>

#include "ps_hdr.h"

extern "C" char*				gettxt (char*, char*);

/*----------------------------------------------------------------------------
 *	Get an internationalized string.  String id's contain both the filename:id
 *	and default string, separated by the FS_CHR character.
 */
char*
GetLocalStr (char* idstr)
{
	char*						sep;
	char*						str;
	char*						p;
	char						buf[BUFSIZ];
	int							i = 0;

   	if ((sep = strchr (idstr, FS_CHR)) != NULL) {
		p = idstr;
		while (*p != FS_CHR) {
			buf[i++] = *p++;  
		}
		buf[i] = '\0';
	
   		str = gettxt (buf, (char*)(sep + 1));
   		return (str);
	}
	return (0);
}

/*----------------------------------------------------------------------------
 *
 */
char*
GetName (char* value)
{
	char*						desc;
	char*						caret;

	if (caret = strchr (value, '^')) {
		*caret = 0;
		desc = gettxt (caret + 1, value);
		if (desc == value) {
			desc = strdup (value);
		}
	}
	else {
		desc = strdup (value);
	}
	return (desc);
}

