#ident	"@(#)debugger:gui.d/common/Panes.C	1.2"

#include "Base_win.h"
#include "Window_sh.h"
#include "Panes.h"
#include "Help.h"
#include "UI.h"

// popup, popdown, selection_type, and check_sensitivity
// are pure virtual functions, but the actual functions are needed for cfront 1.2
void
Pane::popup()
{
}

void
Pane::popdown()
{
}

char *
Pane::get_selection()
{
	return 0;
}

int
Pane::get_selections(Vector *)
{
	return 0;
}

Selection_type
Pane::selection_type()
{
	return SEL_none;
}

void
Pane::deselect()
{
}

int
Pane::check_sensitivity(int)
{
	return 0;
}

void
Pane::copy_selection()
{
}
