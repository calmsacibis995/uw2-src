#ident	"@(#)debugger:libutil/common/curr_frame.C	1.4"
#include "utility.h"
#include "ProcObj.h"
#include "Frame.h"
#include "Proctypes.h"
#include "global.h"
#include "Interface.h"

#include <signal.h>

// get and set current frame

// in all cases, we assume the higher level functions
// have tested for a null ProcObj

int
curr_frame(ProcObj *pobj)
{
	Frame	*cframe;
	int	count = 0;

	cframe = pobj->curframe();
	while((cframe = cframe->caller()) != 0)
	{
		count++;
	}
	return count;
}

// total number of valid frames - 1
// actually, returns highest numbered frame,
// where frames are numbered from 0
int
count_frames(ProcObj *pobj)
{
	Frame	*cframe;
	int	count = 0;

	cframe = pobj->topframe();
	while((cframe = cframe->caller()) != 0)
	{
		count++;
		if (prismember(&interrupt, SIGINT))
		{
			// print_stack released SIGINT
			// delete signal from interrupt
			// set to let print_stack print
			// out what we have found so far
			prdelset(&interrupt, SIGINT);
			break;
		}
	}
	return count;
}

int 
set_frame(ProcObj *pobj, int frameno)
{
	Frame	*cframe;
	int	count;

	count = count_frames(pobj);
	if ((frameno < 0) || (frameno > count))
	{
		printe(ERR_frame_range, E_ERROR, frameno, pobj->obj_name());
		return 0;
	}
	cframe = pobj->topframe();
	while(frameno < count)
	{
		cframe = cframe->caller();
		count--;
	}
	if (!pobj->setframe(cframe))
		return 0;

	if (get_ui_type() == ui_gui)
		printm(MSG_set_frame, (unsigned long)pobj, frameno);
	return 1;
}
