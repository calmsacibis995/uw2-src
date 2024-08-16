#ident	"@(#)debugger:libexp/common/Spaces.C	1.1"

#include "libexp.h"
#include "utility.h"
#include <string.h>

Spaces	spaces;

// Spaces allocates an array of blanks, used by print_rval and print_type
// to manage indentation.  The indentation needed is unlikely to ever get
// even close to 100
char *
Spaces::get_spaces(int n)
{
	if (!spaces)
	{
		if (n > 100)
			maxlen = n;
		else
			maxlen = 100;
		spaces = new char[maxlen+1];
		sprintf(spaces, "%*s", maxlen, "");
	}
	else if (n > maxlen)
	{
		maxlen = n;
		spaces = (char *)realloc(spaces, maxlen+1);
		if (!spaces)
			new_handler();
		sprintf(spaces, "%*s", maxlen, "");
	}

	// fill in the null byte in the previous string, and insert a null byte
	// to get the needed number of blanks
	spaces[null_byte] = ' ';
	spaces[n] = '\0';
	null_byte = n;
	return spaces;
}
