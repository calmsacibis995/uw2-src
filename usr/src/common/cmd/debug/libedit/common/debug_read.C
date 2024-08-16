#ident	"@(#)debugger:libedit/common/debug_read.C	1.13"

// This is the top level read routine which is also
// responsible for fielding events from subject processes.
// It waits on a single character read while fielding SIGUSR1.
// If it catches SIG_INFORM, it goes off and does any appropriate
// actions and reprompts if necessary.
//

#include "utility.h"
#include "Interface.h"
#include "Input.h"
#include "global.h"
#include "Proctypes.h"
#include "Machine.h"
#include "sh_config.h"
#include "edit.h"
// shconfig.h and edit.h must come last since sh_config defines SYSCAL_L and
// edit "defines" DELETE, and they are also enums in Parser.h
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

int
debug_read(int fd, void *buf, unsigned int nchar)
{
	int i;

	PrintaxGenNL = 1;	// arrange for a newline to be
				// generated before output.


	PrintaxSpeakCount = 0;

	for(;;) 
	{
		prdelset(&interrupt, SIG_INFORM);
		prdelset(&interrupt, SIGPOLL);

		// unblock SIG_INFORM and SIGPOLL
		// if not gui, also SIGINT
		sigprocmask(SIG_UNBLOCK, &debug_sset, 0);
		if (PrintaxSpeakCount) 
		{
			// event notification occurred;
			// break out so we can process
			// any associated commands and reprompt
			sigprocmask(SIG_BLOCK, &debug_sset, 0);
			i = 0;
			break;
		}
		i = read(fd, buf, nchar);
		sigprocmask(SIG_BLOCK, &debug_sset, 0);

		if (i != -1 || errno != EINTR)
			break;
		if (prismember(&interrupt, SIGINT))
		{
			prdelset(&interrupt, SIGINT);
			i = 0;
			break;
		}
		if (!prismember(&interrupt, SIG_INFORM) &&
			!prismember(&interrupt, SIGPOLL))
			break;
		if (PrintaxSpeakCount) 
		{
			// event notification occurred;
			// break out so we can process
			// any associated commands and reprompt
			i = 0;
			break;
		}
	}
	if (get_ui_type() == ui_gui)
	{
		// cancel any spurious interrupts
		sigrelse(SIGINT);
		prdelset(&interrupt, SIGINT);
		sighold(SIGINT);
	}
	PrintaxGenNL = 0;
	return i;
}
