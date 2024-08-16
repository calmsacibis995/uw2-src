#ident	"@(#)debugger:libexecon/common/Ev_Notify.C	1.4"

#include "Ev_Notify.h"
#include "Event.h"
#include "StopEvent.h"

// stub functions that interface between ProcObjs and event
// mechanism

int
notify_sig_e_trigger(void *thisptr)
{
	return(((Sig_e *)thisptr)->trigger());
}

int
notify_sys_e_trigger(void *thisptr)
{
	return(((Sys_e *)thisptr)->trigger());
}

int
notify_onstop_e_trigger(void *thisptr)
{
	return(((Onstop_e *)thisptr)->trigger());
}

int
notify_stoploc_trigger(void *thisptr)
{
	return(((StopLoc *)thisptr)->trigger());
}

int
notify_stop_e_clean_foreign(void *thisptr)
{
	return((Stop_e *)thisptr)->remove(0);
}

int
notify_returnpt_trigger(void *thisptr)
{
	return(((Returnpt *)thisptr)->trigger());
}

int
notify_watchframe_start(void *thisptr)
{
	return(((Watchframe *)thisptr)->trigger_start(0));
}

int
notify_watchframe_watch(void *thisptr)
{
	return(((Watchframe *)thisptr)->trigger_watch());
}

int
notify_watchframe_end(void *thisptr)
{
	return(((Watchframe *)thisptr)->trigger_end());
}

