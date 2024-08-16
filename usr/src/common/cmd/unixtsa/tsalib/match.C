#ident	"@(#)unixtsa:common/cmd/unixtsa/tsalib/match.C	1.1"

#include <smsutapi.h>

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMIsWild"
#endif
NWBOOLEAN NWSMIsWild(
		STRING	string)
{
	CHAR ch;

	while ((ch = *string++ & 0x7F) isnt 0)
	{
		switch(ch)
		{
		case ASTERISK:
		case QUESTION:
		case SPERIOD:
		case SASTERISK:
		case SQUESTION:
			return (TRUE);
		}
	}

	return (FALSE);
}

#if defined(DEBUG_CODE)
#undef RNAME
#define RNAME "NWSMWildMatch"
#endif
CCODE NWSMWildMatch(
		STRING	pattern,
		STRING	string)
{
	short l;
	CHAR MatchPeriod, p, s;

	while ((p = (*pattern & 0x7F)) isnt '*')
	{
		s = *string & 0x7F;
		if (p is '?')
		{
			if ((*pattern is SQUESTION) and ((s is 0) or (s is '.')))
			{
				/*special question skip period or end*/
				pattern++;
			}

			else
			{
				if (s is 0)
					return (0); /*no char to match*/
				pattern++;	/*succeed on ? match*/
				string++;
			}
		}

		else if ((*pattern is SPERIOD) and (s is 0))
		{
			/*match  special period to end-of-string*/
			pattern++;
		}

		else if (p isnt s)
			return (0); /*failure on non-match*/

		else if (p is 0)
			return (1);	/*success on match to nulls*/

		else
		{
			/*chars match, but not end yet*/
			pattern++;
			string++;
		}
	}

	/* MUST MATCH AN ASTERISK WILDCARD */
	MatchPeriod = 0;
	while ((*pattern & 0x7F) is '*')
	{
		/* step over asterisks */
		if ((*pattern & 0x80) is 0)
			MatchPeriod = 0xFF;

		pattern++;
	}

	for (l = 0; (string[l] isnt 0) and ((string[l] isnt '.') or MatchPeriod);
			l++);	/*count max characters that may be skipped*/

	p = *pattern & 0x7F;
	while (l >= 0)
	{
		s = string[l] & 0x7F;
		if (((p is s) or (p is '?') or (p is '.'))
				and NWSMWildMatch(pattern, &string[l]))
			return (1);	/*success*/
		l--;
	}

	return (0);
}
