#ident	"@(#)umailsetup:cDebug.C	1.1"
//	cDebug.C

#include	<stdio.h>
#include	<Xm/Xm.h>		//  for True/False definition.

#define CDEBUG_DEFINITION		// define storage for debugging
#include	"cDebug.h"






int
cDebugInit (int debugLevel, char *debugFileName, int argc, int maxLevel)
{
	FILE	*fp;


	if (debugLevel > 0 && debugLevel <= maxLevel)
	{
		if (debugFileName)
		{
			printf ("debugFileName=%s.\n", debugFileName);

			if ((fp = fopen (debugFileName, "w+")) == NULL)
				return (False);
			else
				log = fp;
		}

		if (setvbuf (log, "", _IOLBF, (size_t)0))
		{
			printf ("ERROR: setvbuf() failed for %s.\n",
							debugFileName);
		}

		return (True);
	}

	return (False);


}	//  End cDebugInit ()
