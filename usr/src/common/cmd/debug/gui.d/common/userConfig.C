#ident	"@(#)debugger:gui.d/common/userConfig.C	1.14"

// Parse user's configuration file

#include "config.h"
#include "Menu.h"
#include "Panes.h"
#include "Button_bar.h"
#include "UI.h"
#include "Resources.h"

#include "str.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

enum Ctoken {
	T_invalid,
	T_eof,
	T_window,
	T_auto,
	T_number,
	T_name,
	T_pane,
	T_button,
	T_button_panel,
};

struct Token {
	Ctoken	type;
	long	line;
	char	*name;
	int	val;
};

static Token	curtok;

struct Keyword {
	const char	*kname;
	Ctoken		ktoken;
	int		kaction;
};

// names must be sorted alphabetically
static const Keyword	keytable[] = {
	{  "ANIMATE_DIS",  	T_button,	(int)B_animate_dis },
	{  "ANIMATE_SOURCE",  	T_button,	(int)B_animate_src },
	{  "AUTO",  	T_auto,		0 },
	{  "BUTTONS",  	T_button_panel,		0 },
	{  "COMMAND",	T_pane,		(int)PT_command },
	{  "DELETE",  	T_button,	(int)B_delete },
	{  "DESTROY",  	T_button,	(int)B_destroy },
	{  "DISABLE",  	T_button,	(int)B_disable },
	{  "DISASSEMBLY", T_pane,	(int)PT_disassembler },
	{  "ENABLE",  	T_button,	(int)B_enable },
	{  "EVENT", 	T_pane,		(int)PT_event },
	{  "EXPAND", 	T_button,	(int)B_expand },
	{  "EXPORT", 	T_button,	(int)B_export },
	{  "HALT", 	T_button,	(int)B_halt },
	{  "INPUT", 	T_button,	(int)B_input },
	{  "INTERRUPT",	T_button,	(int)B_interrupt },
	{  "NEXT_INST",	T_button,	(int)B_next_inst },
	{  "NEXT_STMT", T_button,	(int)B_next_stmt },
	{  "PIN_SYM",	T_button,	(int)B_sym_pin },
	{  "POPUP",	T_button,	(int)B_popup },
	{  "PROCESS", 	T_pane,		(int)PT_process },
	{  "REGISTER", 	T_pane,		(int)PT_registers },
	{  "RETURN",  	T_button,	(int)B_return },
	{  "RUN",  	T_button,	(int)B_run },
	{  "SET_CURRENT", T_button,	(int)B_set_current },
	{  "SET_WATCHPOINT", T_button,	(int)B_set_watchpt },
	{  "SOURCE", 	T_pane,		(int)PT_source },
	{  "STACK", 	T_pane,		(int)PT_stack },
	{  "STATUS", 	T_pane,		(int)PT_status },
	{  "STEP_INST",	T_button,	(int)B_step_inst },
	{  "STEP_STMT",	T_button,	(int)B_step_stmt },
	{  "SYMBOLS", 	T_pane,		(int)PT_symbols },
	{  "UNPIN_SYM",	T_button,	(int)B_sym_unpin },
	{  "WINDOW",	T_window,	0 },
	{  0,		T_invalid,	0 }
};

#define MAXKEYLEN sizeof("Animate_Source")

class Lexer {
	int	nodelete;
	const char *fname;
	unsigned char	*buf;
	size_t	bsize;
	char	*namebuf;
	size_t	namesize;
	long	curline;
	unsigned char	*cur;
	unsigned char	key[MAXKEYLEN];
	int	nextc() { 	int	ctmp;
				if ((cur - buf) >= bsize)
					ctmp = -1;
				else
					ctmp = *cur++;
				return ctmp;  }
	void	pushback() { if ((cur - buf) < bsize) cur--; }
public:
		Lexer(int fd, const char *name, size_t size);
		Lexer(const char *config);
		~Lexer();
	void	nexttok();	// assigns to curtok;
	int	is_valid() { return(cur != 0); }
	const char *get_fname() { return fname; }
};

static void
syntax_error(Lexer *lex)
{
	if (lex->get_fname())
		display_msg(E_ERROR, GE_config_file_syntax, 
			lex->get_fname(), curtok.line);
	else
		display_msg(E_ERROR, GE_config_syntax, curtok.line);
}

struct Button_desc {
	const Button_bar_table	*def_desc;
	char			*name;
	char			*window;
};

// format is list of:
// button_type [name]
// For the POPUP button, the format is:
// POPUP name [name]
static void
parse_buttons(Lexer *lex, Window_descriptor *wd)
{
	int			bcount	= 0;
	Button_desc		bd[B_last];
	Button_desc		*bptr = bd;
	Button_bar_table	*btable;
	long			line;

	while((curtok.type == T_button) && (bcount < B_last))
	{
		bcount++;
		line = curtok.line;
		if ((bptr->def_desc = 
			get_button_desc((CButtons)curtok.val)) == 0)
		{
			display_msg(E_ERROR, GE_internal, __FILE__,
				__LINE__);
			return;
		}
		lex->nexttok();
		if (curtok.type == T_name)
		{
			char	*name = makestr(curtok.name);
			if (bptr->def_desc->type == B_popup)
			{
				bptr->window = name;
				lex->nexttok();
				if (curtok.type == T_name)
				{
					bptr->name = makestr(curtok.name);
					lex->nexttok();
				}
				else
					bptr->name = bptr->window;
			}
			else
			{
				bptr->name = name;
				bptr->window = 0;
				lex->nexttok();
			}
		}
		else
		{
			if (bptr->def_desc->type == B_popup)
			{
				
				if (lex->get_fname())
					display_msg(E_ERROR, 
					GE_config_file_popup_label, 
					lex->get_fname(), line);
				else
					display_msg(E_ERROR, 
					GE_config_popup_label, line);
				return;
			}
			bptr->name = 0;
		}
		bptr++;
	}
	if (!bcount)
		return;
	bptr = bd;
	btable = new Button_bar_table[bcount];
	wd->button_table = new Button_bar_table *[bcount];
	wd->nbuttons = bcount;
	for(int i = 0; i <  bcount; i++, bptr++)
	{
		memcpy((void *)&btable[i], bptr->def_desc, 
			sizeof(Button_bar_table));
		wd->button_table[i] = &btable[i];
		if (bptr->name)
			btable[i].label = bptr->name;
		if (bptr->window)
			btable[i].window = bptr->window;
	}
}

struct Pane_desc {
	const Pane_descriptor	*pane;
	int			nolines;
	int			nocolumns;
};

// Format is list of:
// pane_type [rows [columns]]

static void
parse_panes(Lexer *lex, Window_descriptor *wd)
{
	Pane_desc		pd[PT_last];
	Pane_desc		*pptr = pd;
	Pane_descriptor		*parray;
	int			pcount = 0;

	while((curtok.type == T_pane) && (pcount < PT_last))
	{
		pcount++;
		if ((pptr->pane = get_pane_desc((Pane_type)curtok.val)) == 0)
		{
			display_msg(E_ERROR, GE_internal, __FILE__,
				__LINE__);
			return;
		}
		lex->nexttok();
		if (curtok.type == T_number)
		{
			pptr->nolines = curtok.val;
			lex->nexttok();
			if (curtok.type == T_number)
			{
				pptr->nocolumns = curtok.val;
				lex->nexttok();
			}
			else
			{
				pptr->nocolumns = 0;
			}
		}
		else
		{
			pptr->nolines = 0;
			pptr->nocolumns = 0;
		}
		switch(pptr->pane->type)
		{
		default:
			break;
		case PT_command:
			wd->flags |= W_HAS_COMMAND;
			if (pptr->nolines > max_rows)
				max_rows = pptr->nolines;
			break;
		case PT_status:
			wd->flags |= W_HAS_STATUS;
			break;
		case PT_event:
			wd->flags |= W_HAS_EVENT;
			break;
		}
		pptr++;
	}
	if (pcount == 0)
	{
		if (curtok.type == T_window || 
			curtok.type == T_button_panel ||
			curtok.type == T_eof)
		{
			if (lex->get_fname())
				display_msg(E_ERROR, GE_config_file_nopanes, 
					lex->get_fname(), wd->name);
			else
				display_msg(E_ERROR, GE_config_nopanes, 
					wd->name);
		}
		else
			syntax_error(lex);
		wd->npanes = 0;
		wd->panes = 0;
		return;
	}
	wd->panes = new Pane_descriptor *[pcount];
	parray = new Pane_descriptor[pcount];
	pptr = pd;
	for(int i = 0; i < pcount; i++, pptr++)
	{
		memcpy((void *)&parray[i], pptr->pane, 
			sizeof(Pane_descriptor));
		wd->panes[i] = &parray[i];
		if (pptr->nolines)
			parray[i].nlines = pptr->nolines;
		if (pptr->nocolumns)
			parray[i].ncolumns = pptr->nocolumns;
	}
	wd->npanes = pcount;
}

static void
multiple_bar_error(Lexer *lex, Window_descriptor *wptr)
{
	if (lex->get_fname())
		display_msg(E_ERROR, GE_config_file_multiple_buttons, 
			lex->get_fname(), wptr->name);
	else
		display_msg(E_ERROR, GE_config_multiple_buttons, 
			wptr->name);
}

// List of window descriptors;  each window descriptor
// starts with
static Window_descriptor *
parse_windows(Lexer *lex)
{
	Window_descriptor	*wd;
	Window_descriptor	*wptr;
	int			wcount = 0;
	int			wdsize = PT_last;

	wptr = wd = new Window_descriptor[PT_last];

	lex->nexttok();
	while(curtok.type == T_window)
	{
		wcount++;
		if (wcount > wdsize)
		{
			Window_descriptor	*new_wd;
			new_wd = new Window_descriptor[wdsize + 5];
			memcpy(new_wd, wd, sizeof(Window_descriptor) * wdsize);
			delete wd;
			wd = new_wd;
			wptr = &wd[wdsize];
			wdsize += 5;
		}

		memset(wptr, 0, sizeof(Window_descriptor));

		lex->nexttok();
		if (curtok.type != T_name)
		{
			syntax_error(lex);
			goto error;
		}
		wptr->name = makestr(curtok.name);

		lex->nexttok();
		if (curtok.type == T_auto)
		{
			wptr->flags |= W_AUTO_POPUP;
			lex->nexttok();
		}

		if (curtok.type == T_button_panel)
		{
			if (wptr->nbuttons)
			{
				multiple_bar_error(lex, wptr);
				goto error;
			}
			lex->nexttok();
			parse_buttons(lex, wptr);
		}

		parse_panes(lex, wptr);
		if (wptr->npanes == 0)
			goto error;

		if (curtok.type == T_button_panel)
		{
			if (wptr->nbuttons)
			{
				multiple_bar_error(lex, wptr);
				goto error;
			}
			lex->nexttok();
			parse_buttons(lex, wptr);
			wptr->flags |= W_BUTTONS_BOTTOM;
		}
		wptr++;
	}
	if (curtok.type != T_eof)
	{
		syntax_error(lex);
		goto error;
	}

	if (!wcount)
		return 0;
	windows_per_set = wcount;
	return wd;
error:
	wptr = wd;
	for(int i = 0; i < wcount; i++, wptr++)
	{
		delete wptr->panes;
		delete wptr->button_table;
		delete (char *)wptr->name;
	}
	delete wd;
	return 0;
}


// Check for duplicate panes (in same window).
// Check for duplicate buttons and for invalid buttons.
// Allocate a window for any unallocated pane (except status).
static Window_descriptor *
check_windows(Window_descriptor *wd, const char *fname)
{

	unsigned char		tcount[PT_last];
	Window_descriptor	*wptr = wd;
	const Window_descriptor	*extra[PT_last];
	const Window_descriptor	**eptr;
	int			wcount = 0;
	int			has_auto = 0;
	int			i;

	for(i = 0; i < (int)PT_last; i++)
		tcount[i] = 0;

	for(i = 0; i < windows_per_set; i++, wptr++)
	{
		unsigned char		lcount[PT_last];
		unsigned char		bcount[B_last];

		Pane_descriptor	**pd = wptr->panes;
		Button_bar_table **bd = wptr->button_table;
		int	j;

		if (wptr->flags & W_AUTO_POPUP)
			has_auto++;
		for(j = 0; j < (int)PT_last; j++)
			lcount[j] = 0;
		for(j = 0; j < (int)B_last; j++)
			bcount[j] = 0;
		for(j = 0; j < wptr->npanes; j++, pd++)
		{
			Pane_descriptor	*pn = *pd;
			if (lcount[pn->type])
			{
				if (fname)
					display_msg(E_ERROR, 
					GE_config_file_duplicate_panes, 
					fname,
					pn->name, wptr->name);
				else
					display_msg(E_ERROR, 
					GE_config_duplicate_panes, 
					pn->name, wptr->name);
				goto clean;
			}
			lcount[pn->type] = 1;
			tcount[pn->type]++;
		}
		for(j = 0; j < wptr->nbuttons; j++, bd++)
		{
			int	error = 0;
			Button_bar_table *btn = *bd;
			if ((btn->type != B_popup) && bcount[btn->type])
			{
				// multiple popup buttons allowed
				if (fname)
					display_msg(E_ERROR, 
					GE_config_file_duplicate_buttons, 
					fname,
					btn->name(), wptr->name);
				else
					display_msg(E_ERROR, 
					GE_config_duplicate_buttons, 
					btn->name(), wptr->name);
				goto clean;
			}
			bcount[btn->type] = 1;
			switch(btn->type)
			{
			case B_set_current:
				if (!lcount[PT_stack] &&
					!lcount[PT_process])
					error = 1;
				break;
			case B_animate_src:
				if (!lcount[PT_source])
					error = 1;
				break;
			case B_animate_dis:
				if (!lcount[PT_disassembler])
					error = 1;
				break;
			case B_disable:
			case B_enable:
			case B_delete:
				if (!lcount[PT_event])
					error = 1;
				break;
			case B_input:
			case B_interrupt:
				if (!lcount[PT_command])
					error = 1;
				break;
			case B_sym_pin:
			case B_sym_unpin:
			case B_set_watchpt:
			case B_export:
			case B_expand:
				if (!lcount[PT_symbols])
					error = 1;
				break;
			case B_popup:
			{
				// button label must match one of
				// the window titles
				Window_descriptor	*w2 = wd;
				int			k = 0;
				for(; k < windows_per_set; k++, w2++)
				{
					if (strcmp(btn->window, w2->name)
						== 0)
						break;
				}
				if (k >= windows_per_set)
				{
					if (fname)
						display_msg(E_ERROR,
						GE_config_file_popup_match,
						fname, btn->window);
					else
						display_msg(E_ERROR,
						GE_config_popup_match,
						btn->window);
					goto clean;
				}
				btn->cdata = k;
				btn->sensitivity = SEN_all_but_script;
				break;
			}
			default:
				break;
			}
			if (error)
			{
				if (fname)
					display_msg(E_ERROR, 
						GE_config_file_button, 
						fname,
						btn->name(), wptr->name);
				else
					display_msg(E_ERROR, 
						GE_config_button, 
						btn->name(), wptr->name);
				goto clean;
			}
		}
	}
	if (!has_auto)
		// no window marked as auto popup - set the first
		// one
		wd->flags |= W_AUTO_POPUP;

	if (tcount[PT_command] > 1)
		cmd_panes_per_set = tcount[PT_command];
	eptr = extra;
	for(i = 0; i < (int)PT_last; i++)
	{
		if (tcount[i] == 0 && ((Pane_type)i != PT_status))
		{
			// allocate new window for missing pane
			wcount++;
			*eptr = get_win_desc((Pane_type)i);
			eptr++;
		}
	}
	if (wcount)
	{
		Window_descriptor	*wtmp;
		wtmp = new Window_descriptor[windows_per_set+wcount];
		memcpy(wtmp, wd, windows_per_set * sizeof(Window_descriptor));
		wptr = &wtmp[windows_per_set];
		windows_per_set += wcount;
		for(i = 0; i < wcount; i++, wptr++)
			memcpy(wptr, extra[i], sizeof(Window_descriptor));
		delete wd;
		return wtmp;
	}
	return wd;
clean:
	for(i = 0, wptr = wd; i < windows_per_set; i++, wptr++)
	{
		delete wptr->button_table;
		delete (char *)wptr->name;
		delete wptr->panes;
	}
	delete wd;
	return 0;
}


// given configuration string 
Lexer::Lexer(const char *config)
{
	nodelete = 1;
	fname = 0;
	bsize = strlen(config);
	buf = (unsigned char *)config;
	cur = buf;
	curline = 1;
	namesize = 100;
	namebuf = new char[namesize];
}

// given configuration file
Lexer::Lexer(int fd, const char *name, size_t size)
{
	nodelete = 0;
	bsize = size;
	fname = name;
	buf = new unsigned char[bsize];
	if (read(fd, buf, bsize) != bsize)
	{
		cur = 0;
		display_msg(E_ERROR, GE_config_read, name);
		return;
	}
	cur = buf;
	curline = 1;
	namesize = 100;
	namebuf = new char[namesize];
}

Lexer::~Lexer()
{
	delete namebuf;
	if (!nodelete)
		delete buf;
}

static const Keyword *
getkey(unsigned char *name)
{
	const Keyword	*k = &keytable[0];
	for(; k->kname != 0; k++)
	{
		int	i;
		i = strcmp((char *)name, k->kname);
		if (i == 0)
			return k;
		else if (i < 0)
			break;
	}
	return 0;
}

void
Lexer::nexttok()
{
	int		c;
	unsigned char	*first;
	unsigned char	*kptr;
	int		count;

	c = nextc();

	while(isspace(c))
	{
		if (c == '\n')
			curline++;
		c = nextc();
	}
	
	while(c == '#')
	{
		// comment
		do {
			c = nextc();
		} while(c != '\n' && c != -1);
		while(isspace(c))
		{
			if (c == '\n')
				curline++;
			c = nextc();
		}
	}

	if (c == -1)
	{
		curtok.type = T_eof;
		return;
	}

	if (isdigit(c))
	{
		int	val;

		val = c - '0';
		c = nextc();
		while(isdigit(c))
		{
			val *= 10;
			val += c - '0';
			c = nextc();
		}
		pushback();
		curtok.val = val;
		curtok.type = T_number;
		curtok.line = curline;
		return;
	}
	else if (c == '\'' || c == '\"')
	{
		// name
		int	quote;

		quote = c;
		first = cur;
		count = 0;
		c = nextc();
		while(c != quote)
		{
			if (c == -1 || c == '\n')	
			{
				curtok.type = T_invalid;
				curtok.line = curline;
				return;
			}
			count++;
			c = nextc();
		}
		if (count >= namesize)
		{
			delete namebuf;
			namesize = count + 20;
			namebuf = new char[namesize];
		}
		strncpy((char *)namebuf, (const char *)first, count);
		namebuf[count] = 0;
		curtok.name = namebuf;
		curtok.line = curline;
		curtok.type = T_name;
		return;
	}
	else if (!isalpha(c))
	{
		curtok.type = T_invalid;
		curtok.line = curline;
		return;
	}
	// keyword
	kptr = key;
	*kptr++ = toupper(c);
	c = nextc();
	count = 1;
	while(isalpha(c) || (c == '_'))
	{
		count++;
		if (count >= MAXKEYLEN)
		{
			break;
		}
		*kptr++ = toupper(c);
		c = nextc();
	}
	pushback();
	if (count < MAXKEYLEN)
	{
		const Keyword	*k;
		key[count] = 0;
		if ((k = getkey(key)) != 0)
		{
			curtok.type = k->ktoken;
			curtok.val = k->kaction;
			curtok.line = curline;
			return;
		}
	}
	curtok.type = T_invalid;
	curtok.line = curline;
}

Window_descriptor *
read_user_file()
{
	const char		*path;
	const char		*config;
	struct stat		sbuf;
	int			fd = -1;
	Lexer			*lex;
	Window_descriptor	*wd;

	if ((path = resources.get_config_file()) != 0)
	{
		if ((fd = open(path, O_RDONLY)) == -1)
		{
			display_msg(E_ERROR, GE_config_open, path,
				strerror(errno));
			return 0;
		}
		if ((fstat(fd, &sbuf) == -1) ||
			(sbuf.st_size == 0))
		{
			display_msg(E_ERROR, GE_config_stat, path);
			close(fd);
			return 0;
		}
		lex = new Lexer(fd, path, (size_t)sbuf.st_size);
		close(fd);
	}
	else if ((config = resources.get_config()) != 0)
	{
		lex = new Lexer(config);
		path = 0;
	}
	else
	{
		return 0;
	}

	if (!lex->is_valid())
	{
		delete lex;
		return 0;
	}
	wd = parse_windows(lex);
	delete lex;
	if (!wd)
		return 0;
	return check_windows(wd, path);
}
