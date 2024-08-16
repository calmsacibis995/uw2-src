#ident	"@(#)debugger:config.d/common/config.cmd	1.1"

# In this configuration, the main window contains a source
# pane and a command pane.  The stack, symbols, process and event panes
# are each in their own windows.  The disassembly window contains
# the registers and disassembly panes.

Window "Source" auto 
	Buttons 
		Run 
		Return 
		Step_Stmt 
		Next_Stmt 
		Halt 
		Destroy 
		Popup "Stack"
		Popup "Symbols"
	Source	10 60 
	Command 5 60
Window "Disassembly" 
	Buttons 
		Run 
		Return 
		Step_Inst 
		Next_Inst 
		Halt 
		Destroy 
		Popup "Process"
		Popup "Stack"
		Popup "Source"
	Status	1
	Register	6  70
	Disassembly	10 70
Window "Process" 
	Buttons 
		Set_current 
		Popup "Disassembly" "Dis"
		Popup "Event"
		Popup "Source"
		Popup "Stack"
		Popup "Symbols"
	Process 5 
Window "Symbols" 
	Buttons 
		Export 
		Pin_Sym	 "Pin"
		Unpin_Sym  "Unpin"
		Set_Watchpoint  "Set Watch"
		Popup "Disassembly" "Dis"
		Popup "Event"
		Popup "Process"
		Popup "Source"
		Popup "Stack"
	Status	1 
	Symbols	8 
Window "Event" 
	Buttons Disable Enable Delete
		Popup "Disassembly" "Dis"
		Popup "Process"
		Popup "Source"
		Popup "Stack"
		Popup "Symbols"
	Status	1 
	Event	5 
Window "Stack"
	Buttons 
		Set_current
		Popup "Disassembly" "Dis"
		Popup "Event"
		Popup "Process"
		Popup "Source"
		Popup "Symbols"
	Status	1
	Stack	5
