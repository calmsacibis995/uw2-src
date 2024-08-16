#ident	"@(#)unixtsa:common/cmd/unixtsa/tsalib/free.C	1.1"

#include <smsutapi.h>

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMFreeNameList"
#endif
CCODE NWSMFreeNameList(
		NWSM_NAME_LIST	**list)
{
	CCODE ccode = 0;
	NWSM_NAME_LIST *ptr, *next;

	if (!list)
	{
		ccode = NWSMUT_INVALID_PARAMETER;
		goto Return;
	}

	if (*list)
	{
		for (ptr = *list; ptr; ptr = next)
		{
			next = ptr->next;

			free((char *)ptr->name);
			free((char *)ptr);
		}

		*list = NULL;
	}

Return:
	return (ccode);
}

