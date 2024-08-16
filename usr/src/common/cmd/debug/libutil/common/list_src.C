#ident	"@(#)debugger:libutil/common/list_src.C	1.12"

#include "utility.h"
#include "SrcFile.h"
#include "ProcObj.h"
#include "Interface.h"
#include "Location.h"
#include "Proglist.h"
#include "global.h"
#include "Process.h"

#ifdef OLD_REGEXP
#include <regexpr.h>
#else
#include <regex.h>
#endif

static int	fprintn(ProcObj *pobj, SrcFile *, long);
static SrcFile	*check_srcfile(SrcFile *, ProcObj *, char *, 
			long, long &);
static int	dore(ProcObj *, SrcFile *, const char *, int);

int 
list_src(Proclist *procl, int count, Location *l, const char *re, int mode)
{
	long		lcount = (count > 0) ? count : num_line;
	int		ret = 1;
	int 		single = 1;
	ProcObj		*pobj;
	plist		*list;

	if (procl)
	{
		single = 0;
		list = proglist.proc_list(procl);
		pobj = list++->p_pobj;
	}
	else
	{
		pobj = proglist.current_object();
	}
	// if a pobj is specified in the location, it overrides
	if (l)
	{
		ProcObj	*l_pobj;
		if (!l->get_pobj(l_pobj))
		{
			return 0;
		}
		if (l_pobj)
		{
			pobj = l_pobj;
			single = 1;
		}
	}
	if (!pobj)
	{
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}
	sigrelse(SIGINT);
	do {
		SrcFile		*sf;
		char		*file;
		char		*func;
		char		*name;
		long		line;
		Frame		*f;

		sf = 0;
		if (prismember(&interrupt, SIGINT))
			break;

		if (!single)
			printm(MSG_list_header, pobj->obj_name(), pobj->prog_name());
		if (!pobj->state_check(E_DEAD))
		{
			ret = 0;
			continue;
		}
		if (l)
		{
			f = pobj->curframe();

			if (l->get_file(pobj, f, file) == 0)
			{
				ret = 0;
				continue;
			}
			if (!file) 
			{
				// list on running process allowed
				// only where fully qualified locaction
				// is given
				if (!pobj->state_check(E_RUNNING|E_DEAD))
				{
					ret = 0;
					continue;
				}
			}
			switch(l->get_type())
			{
				case lk_none:
				case lk_addr:
				default:
					printe(ERR_bad_list_loc, E_ERROR);
					ret = 0;
					goto out;
				case lk_fcn:
					if ((l->get_func(pobj, f, func) == 0) || 
						(func == 0))
					{
						ret = 0;
						continue;
					}
					if ((name = find_fcn(pobj, file,
						func, line )) == 0)
					{
						ret = 0;
						continue;
					}
					break;
				case lk_stmt:
					if (!file)
					{
						name = pobj->curr_src();
					}
					else 
						name = file;
					if (l->get_line(pobj, f, 
						(unsigned long &)line) == 0)
					{
						ret = 0;
						continue;
					}
			}
			if ((sf = check_srcfile(0, pobj, name, 
				line, lcount)) == 0)
			{
				ret = 0;
				continue;
			}
			pobj->set_current_stmt(name, line);
		}
		else
		{
			if (!pobj->state_check(E_RUNNING))
			{
				ret = 0;
				continue;
			}
			if (re)
			{
				long	tmp = 1;
				// make sure we have current line
				if ((sf = check_srcfile(0, pobj,
					pobj->curr_src(), 
					pobj->current_line(), tmp)) == 0)
				{
					ret = 0;
					continue;
				}
				if (!dore(pobj, sf, re, mode))
				{
					ret = 0;
					continue;
				}
				if (count <= 0)
				{
					long currline = pobj->current_line();
					printm(MSG_line_src, currline,
						sf->line((int)currline));
					continue;
				}
			}
			if ((sf = check_srcfile(sf, pobj, 
				pobj->curr_src(), pobj->current_line(), 
				lcount)) == 0)
			{
				ret = 0;
				continue;
			}
		}
		if (!fprintn(pobj, sf, lcount))
		{
			ret = 0;
		}
		if (!single)
			printm(MSG_newline);
	} 
	while(!single && ((pobj = list++->p_pobj) != 0));
out:
	sighold(SIGINT);
	return ret;
}

static SrcFile *
check_srcfile(SrcFile *sf, ProcObj *pobj, char *fname,
	long line, long &num)
{
	long	hi;

	if (!sf)
	{
		if (fname == 0 || *fname == 0)
		{
			printe(ERR_no_cur_src_obj, E_ERROR, 
				pobj->obj_name());
			return 0;
		}
		if ((sf = find_srcfile( pobj, fname )) == 0) 
		{
			printe(ERR_no_source_info, E_ERROR, fname);
			return 0;
		}
	}
	if (line <= 0)
	{
		printe(ERR_bad_line_number, E_ERROR);
		return 0;
	}
	if ((num = sf->num_lines(line, num, hi)) == 0)
	{
		if (hi == 0)
			printe(ERR_no_lines, E_ERROR, sf->filename());
		else
			printe(ERR_only_n_lines, E_ERROR, hi,
				sf->filename());
		return 0;
	}
	return sf;
}

// Print count lines.
static int
fprintn(ProcObj *pobj, SrcFile *sf, long count )
{
	long	firstline, lastline;
	char	*fname;

	if ((fname = pobj->curr_src()) == 0)
	{
		printe(ERR_no_cur_src_obj, E_ERROR, pobj->obj_name());
		return 0;
	}
	// firstline would be zero for a file compiled w/o -g - 
	// no line number info
	if ((firstline = pobj->current_line()) == 0)
		firstline = 1;
	lastline = firstline + count - 1;

	while (firstline <= lastline)
	{
		if (prismember(&interrupt, SIGINT))
			break;
		printm(MSG_line_src, firstline, sf->line((int)firstline));
		firstline++;
	}
	pobj->set_current_stmt(fname, lastline);

	return count;
}

// regular expression parsing and searching
static int
dore(ProcObj *pobj, SrcFile *sf, const char *nre, int forward)
{
	long 			cline, tline;
	char			*fname;
#ifdef OLD_REGEXP
	static char		*re;
#else
	static regex_t		rex;
	static regex_t		*re;
#endif

	if ((fname = pobj->curr_src()) == 0)
	{
		printe(ERR_no_cur_src_obj, E_ERROR, pobj->obj_name());
		return 0;
	}

	if (*nre != '\0')
	{
#ifdef OLD_REGEXP
		if (re)
			free(re);
		if ((re = compile(nre, 0, 0)) == 0)
#else
		if (re)
			regfree(re);
		else
			re = &rex;
		if (regcomp(re, nre, REG_NOSUB) != 0)
#endif
		{
			printe(ERR_bad_regex, E_ERROR, nre);
			re = 0;
			return 0;
		}
	}
	else if (!re)
	{
		printe(ERR_no_previous_re, E_ERROR);
		return 0;
	}

	tline = cline = pobj->current_line();
	do {
		long	hi;
		if (prismember(&interrupt, SIGINT))
			break;
		if (forward)
		{
			if (sf->num_lines(tline, 2, hi) == 1)
				// last line
				tline = 1;
			else
				tline += 1;
		}
		else
		{
			if (tline == 1)
				// go to last line
				tline = sf->num_lines(0, 0, hi);
			else
				tline -= 1;
		}
#ifdef OLD_REGEXP
		if (step(sf->line((int)tline), re))
#else
		if (regexec(re, sf->line((int)tline), 0, 0, 0) == 0)
#endif
		{
			pobj->set_current_stmt(fname, tline);
			return 1;
		}
	} while (tline != cline);
	printe(ERR_no_re_match, E_WARNING);
	return 0;
}
