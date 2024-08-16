#ident	"@(#)debugger:libexecon/common/Siglist.C	1.3"

#include "Siglist.h"
#include "Ev_Notify.h"
#include "Machine.h"
#include "Proctypes.h"
#include "Proglist.h"
#include <signal.h>
#include <string.h>
#include <sys/types.h>

Siglist::Siglist()
{
	// start off catching all signals
	prfillset(&_sigctl.signals);
	prfillset(&_sigset);
#ifdef DEBUG_THREADS
	prdelset(&_sigctl.signals, SIGLWP);
	prdelset(&_sigctl.signals, SIGWAITING);
	prdelset(&_sigset, SIGLWP);
	prdelset(&_sigset, SIGWAITING);
#endif
	memset((void *)_events, 0, (NSIG-1) * sizeof(NotifyEvent *));
}

Siglist::~Siglist()
{
	for ( int i = 0 ; i < NSIG-1 ; i++ )
	{
		NotifyEvent	*ne = _events[i];
		while(ne)
		{
			NotifyEvent	*tmp;
			tmp = ne;
			ne = ne->next();
			delete(tmp);
		}
	}
}

NotifyEvent *
Siglist::events( int sig )
{
	if ( (sig <= 0) || (sig >= NSIG) )
		return 0;
	return _events[sig-1];
}

// add a signal event; does not affect the signal catch/ignore
// settings.
// returns 1 if the _sigctl changes; else 0
int
Siglist::add( sigset_t *sigs, Notifier func, void *thisptr, ProcObj *p)
{
	int	changed = 0;

	for (int i = 1; i < NSIG; i++)
	{
		if (prismember(sigs, i))
		{
			NotifyEvent	*ne; 
			ne = new NotifyEvent(func, thisptr, p);
			if (_events[i-1])
				ne->prepend(_events[i-1]);
			_events[i-1] = ne;
			if (!prismember(&_sigctl.signals, i))
			{
				changed = 1;
				praddset(&_sigctl.signals, i);
			}
		}
	}
	return changed;
}

// delete a signal event; does not affect the signal catch/ignore
// settings.
// returns 1 if _sigctl changes, else 0
int
Siglist::remove( sigset_t *sigs, Notifier func, 
	void *thisptr, ProcObj *p )
{
	NotifyEvent	*el;
	int		changed = 0;

	for (int i = 1; i < NSIG; i++)
	{
		if (prismember(sigs, i))
		{
			el = _events[i-1];
			if (el)
			{
				NotifyEvent	*ne = el;
				for(; ne; ne = ne->next())
				{
					if ((ne->func == func) &&
						(ne->object == p) &&
						(ne->thisptr == thisptr))
					break;
				}
				if (!ne)
					return 0;
				if (ne == el)
				{
					_events[i-1] = ne->next();
				}
				ne->unlink();
				delete(ne);
				if (!el && !prismember(&_sigset, i))
				{
					// ignored and no more events
					prdelset(&_sigctl.signals, i);
					changed = 1;
				}
			}
		}
	}
	return changed;
}

// set global process state to catch signals.
// if _sigctl changes, returns 1, else 0.
// catch requests at the thread level do not
// affect the process catch set, but may
// affect _sigctl.
int
Siglist::catch_sigs(sigset_t *sigs, int level)
{
	int		changed = 0;

	for (int i = 1; i < NSIG; i++)
	{
		if (prismember(sigs, i))
		{
			if (level > P_THREAD)
				praddset(&_sigset, i);
			if (!prismember(&_sigctl.signals, i))
			{
				praddset(&_sigctl.signals, i);
				changed = 1;
			}
		}
	}
	return changed;
}

// ignore signals.
// if _sigctl changes, returns 1, else 0.
// thread level requests do not get here.
int
Siglist::ignore_sigs(sigset_t *sigs)
{
	int		changed = 0;

	for (int i = 1; i < NSIG; i++)
	{
		if (prismember(sigs, i))
		{
			prdelset(&_sigset, i);
			if (prismember(&_sigctl.signals, i) &&
				!_events[i-1])
			{
				prdelset(&_sigctl.signals, i);
				changed = 1;
			}
		}
	}
	return changed;
}

void
Siglist::copy(Siglist &olist)
{
	memcpy((char *)&_sigset, (char *)&olist._sigset,
		sizeof(sigset_t));
	memcpy((char *)&_sigctl, (char *)&olist._sigctl,
		sizeof(sig_ctl));
}
