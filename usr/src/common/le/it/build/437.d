$ SCSIDM @(#)messages	1.4    LCC     ;  Modified: 18:56:55 2/14/92
$domain LCC.MERGE.DOS.MERGE
$quote "
$ The following messages are from merge.c
$ 
MERGE2 "%1: %2\n"
MERGE3 "%1: %2\n"
MERGE4 "p->nome: %1\n"
MERGE5 "p->indir: %1\n"
MERGE6 "p->offsetb: %1\n"
MERGE7 "p->dimb: %1\n"
MERGE8 "p->veritÖ: %1\n"
MERGE9 "p->tipo: %1\n"
MERGE10 "MERGE: %1"
MERGE11 "MERGE: sintassi:\n"
MERGE12 "    merge set <nome-token> <valore>\n"
MERGE13 "    merge display [<nome-token>]\n"
MERGE14 "    merge return <nome-token>\n"
MERGE15 "p->commento: %1\n"
MERGE16	"\nnomi di token validi sono:\n"
$ These are error messages from merge.c
BAD_NAME "impossibile trovare nome-token.\n"
TOOBIG "valore troppo grande.\n"
BAD_VALUE "il valore deve essere numerico.\n"
BAD_FILE "Impossibile aprire il file.\n"
USE_ONOFF "va usato on|off.\n"
USE_LOCREM "va usato remote|local\n"
BAD_SET "valori non validi nel file merge.set\n"
SET_MSG "set"
ON_MSG "sç"
OFF_MSG "no"
REMOTE_MSG "remoto"
LOCAL_MSG "locale"
DISPLAY_MSG "schermo"
RETURN_MSG "return"
$	SCCSID(@(#)messages	9.2	LCC)	; Modified: 15:55:17 3/28/94
$domain LCC.PCI.DOS.ON

$quote "
$ The following messages are from on.c
$ NOTE: on.c is also the source for jobs and kill

$ this is the legend bar for the jobs command
ON1 "LAVORO  HOST      STATO   STATO USCITA   COMANDO\n"

$ this is the display format for the jobs command
$ if you change the order here be sure to change the
$ legend bar, ON1.
$ The values here are:
$	%1 job number
$	%2 host drive
$	%3 host name
$	%4 job state (running, sleeping, killed, etc.)
$	%5 exit status as text
$	%6 command
$ NOTE: it is unlikely that this format will need to be changed
ON2 "[%1]  %2 %3  %4 %5  %6\n"

$ %1 is the name of the current command running
ON3 "%1\n"

$ this is for detached processes. %1 is the job index.
ON4 "[%1] staccato\n"

$ this message is used if a job cannot be run, %1 is the program
$ name (on, jobs, kill) and %2 is the command give.
ON5 "%1: %2: accesso negato o file non trovato\n"

$ this is the first part of an error message, %1 is the program name
ON6 "%1: "

$ %1 is the program name
ON7 "%1: nessun processo remoto nella tabella dei lavori."


ON8 "Segnali ammessi:\n\t"

$ this is the format for the output of each allowable signal,
$ %1 is the signal name
ON9 "%1, "

ON12 "%1: Impossibile annullare %2\n"

$ these two messages define the user's options
$ abort check defines which characters to match
$ for each of the three options both in upper
$ and lower case
ON13 "a - interrompere, c - continuare, s - staccare: "
$ NOTE: ABORT_CHECK must have both upper and lower for each option,
$ immediately following each other or the program will behave
$ unexpectedly
ABORT_CHECK	 "AaCcSs"

ON16 "BRIDGE.DRV non installato o versione incompatibile"
ON18 "versione incompatibile di BRIDGE.DRV"

$ %1 is either the drive letter, the hostname or a '-'
ON19 "<%1> non ä un host connesso"

$ drive %1 is not a virtual drive 
ON20 "<%1>: non ä un'unitÖ virtuale"

$ program %1 received a bad numeric argument %2
ON21 "%1: argomento numerico non valido (%2)\n"

$ %1 is the name of the jobs program
ON22 "Uso: %1 [%%job o -]\n"

$ %1 is the name of the jobs program
ON23 "%1: lavoro non trovato"

ON24 "o un argomento numerico.\n"

$ %1 is the name of the kill program
ON25 "%1: Deve indicare un lavoro o una id di processo.\n"

$ %1 is the name of the jobs command, %2 is the job number or id given
ON26 "%1: lavoro %2 non trovato\n"

$ %1 is the program name, %2 is an error output, one of the ERROR
ON27 "%1\n"

$ %1 is the number of the unknown error
ON28 "Errore di uexec sconosciuto (%1)\n"

$ %1 is the name of the on program
ON29 "Uso: %1 [-segnale] %jobid\n"

$ %1 is the name of the program
ON30 "%1: tabella lavori piena\n"

ON31 "Versione DOS incompatibile."

$ %1 is the name of the program
ON32 "Uso: %1 <comandohost>\n"
ON33 "Uso: %1 <host> <comando>\n"

$ %1 is the name of the program %2 is the lcs_errno encountered in the
$ failed translation
ON34 "%1: Errore di traduzione %2.\n"

ON35 "File LMF file \"ABORT_CHECK\" mal specificato\n"
ON36 "Impossibile creare il file temporaneo %1\n"

$ shared usage message
ONUSAGE "       %1 /h o /? mostra questo messaggio\n"

$ internal variables that need to be patched for on.c
$ NOTE: these names were placed here by hand!

$ these are the possible named signals
SIGNAME0	"uccidere"
SIGNAME1	"term"
SIGNAME2	"usr1"
SIGNAME3	"usr2"

$ these are the possible jobstates
JOBSTATES0	"Sconosciuto"
JOBSTATES1	"In esecuzione"
JOBSTATES2	"Fine"
	
$ these are the possible exit statuses
EXTYPES0    	"uscita"
EXTYPES1	"segnale"
EXTYPES2	"copianucleo"
EXTYPES3	"err3"
EXTYPES4	"sconosciuto"

$ these are the failure error messages
ERRVEC0		"Errore in servizio di rete."
ERRVEC1		"Non connesso a un host."
ERRVEC2		"Exec dell'host fallita."
ERRVEC3		"Formato non valido."
ERRVEC4		"Errore di allocazione della memoria del DOS."
$ SCCSID(@(#)messages	7.7 changed 1/28/92 19:37:36)
$domain LCC.PCI.DOS.LOGOUT
$quote "

$ The following messages are from logout.c
LOGOUT1 "Eseguito il logout di tutte le unitÖ virtuali.\n"

$ This is the virtual drive identifier of the host
LOGOUT2 "%1: "

$ This is the name of the host
LOGOUT3 " (%1)"

$ This indicates that the host to be logged out was not logged in
LOGOUT4 " non eseguito il login a\n"

$ The host has been logged out
LOGOUT5 "eseguito il logout\n"

$ This is the usage message for the logout program
LOGOUT6 "\
Uso:\n\
      %1            - tutti gli host\n\
  o   %2 <nomehost> - host indicato\n\
  o   %3 <unitÖ>:   - host collegato all'unitÖ\n\
  o   %4 /h o /?    - stampa questo messaggio\n\
"

$ The logout program has run out of memory
LOGOUT7 "memoria esaurita"

$domain LCC.PCI.DOS.PCICONF
$ The following messages are from pciconf.c

$ This message is used to print an error message from pciconf
PCICONF1 "PCICONF: %1"

$ This is the usage message 
PCICONF2 "PCICONF: sintassi:\n\
    pciconf set nome-token [on|off][remote|local]\n\
    pciconf display [nome-token]\n\
    pciconf return nome-token\n\
    pciconf /h o /?  stampa questo messaggio\n"

$ These messages are the commands that may be given as arguments to pciconf
PCICONF3 "set"
PCICONF4 "schermo"
PCICONF5 "return"

$ These messages are the names of values the options can have
VAL_ON     "sç"
VAL_OFF    "no"
VAL_LOCAL  "locale"
VAL_REMOTE "remoto"

$ These messages are the token-names
$ (they will be truncated to 14 characters)
O_NETBIOS "netbios"
O_NETDR   "vdrive"
O_5F02    "5f02"
O_LPTBIN  "lpt_binary"

$ This messages precedes the list of possible token names 
PCICONF6 "\nPossibili nomi di token sono:\n"

$ This message formats the possible tokens
PCICONF7 "\t%1\n"

$ This tells the user that he has selected an invalid option (on/off are valid)
PCICONF8 "va usato on|off.\n"

$ This tells the user that he has selected an invalid option 
$ (remote/local are valid)
PCICONF9 "va usato remote|local\n"
PCICONF10 "memoria o file di programma danneggiati\n"
PCICONF12 "nome di token sconosciuto.\n"

$ these are debug messages only
PCICONF15 "debug on\n"
PCICONF16 "SET\n"
PCICONF17 "DISPLAY <NOME>\n"
PCICONF18 "RETURN\n"
PCICONF19 "Errore: azione non valida: %1\n"

$ These are used to list the values of local/remote drive and nbs,nd on/off 
PCICONF22 "%1: %2\n"
PCICONF23 "sç"
PCICONF24 "no"
PCICONF25 "remoto"
PCICONF26 "locale"

$domain LCC.PCI.DOS.PCIINIT
$ The following messages are from pciinit.c
PCIINIT1 "non trovato dispositivo BRIDGE corretto\n"

$ The following shows an initialization error and the associated errno
$ PCIINIT2 is a discontinued message
PCIINIT2 "errore di inizializzazione $ %1\n"

PCIINIT_BAD_DOS     "errore: inizializzazione non effettuata, DOS irriconoscibile\n"
PCIINIT_BAD_BRIDGE  "errore: inizializzazione non effettuata, BRIDGE.DRV NON VALIDO\n"
PCIINIT_DRIVER_VER  "avviso: Versione del driver di rete non corretta\n"
PCIINIT_DRIVER_INIT "avviso: inizializzazione del driver di rete non riuscita\n"
PCIINIT_NO_NET \\x13
      "errore: non inizializzato, Manca interfaccia di rete o porta RS232\n"

PCIINIT_FOR_NET   "per l'uso tramite interfaccia di rete\n"
PCIINIT_FOR_RS232 "per l'uso tramite porta RS232\n"
PCIINIT_FOR_BOTH  "per l'uso tramite interfacce di rete e porte RS232\n"

PCIINIT_DRIVES    "Le unitÖ di PC-Interface vanno da %1 a %2\n"
PCIINIT_NETWARE_DRIVES "La prima unitÖ NetWare ä %1\n"

PCIINIT3 "avviso: impossibile impostare il nome del computer\n"

$ An improperly formed (one without a leading '-' or '/' ) was given.
PCIINIT4 "argomento non valido: <%1>\n"

$ An invalid option was given
PCIINIT5 "opzione <%1> sconosciuta\n"

$ The broadcast address given is not in the proper format 
PCIINIT6 "indirizzo di diffusione <%1> non valido\n"

$ there was no 'localhost' internet address found for the PC
PCIINIT7 "Voce '%1' non trovata\n"

$ the localhost address is not in the proper format or invalid
PCIINIT8 "indirizzo locale <%1> non valido\n"

$  The address does not conform to the internet Class A, B or C values
PCIINIT9 "Classe di indirizzi \"%1\" non valida\n"

$ Both the EXCELAN and the HOSTS environment variables must agree
PCIINIT10 "\"HOSTS\" e \"EXCELAN\\tcp\" devono concordare\n"

PCIINIT11 " impossibile aprire %1\n"
PCIINIT12 "la versione di bridge deve essere successiva alla 2.7.2\n"

$ The next two messages refer to the HOSTS file
PCIINIT13 "Impossibile aprire il file %1.\n"
PCIINIT14 "%1 ä danneggiato o non valido.\n"


PCIINIT15 "unitÖ remota: %1\n"
PCIINIT16 "Release %1.%2.%3 (Numero di serie %4.%5) Inizializzata\n"

$ this prints the ip address
PCIINIT17 "%1"
PCIINIT18 "Indirizzo IP = "
PCIINIT19 "\nIndirizzo di diffusione = "

$ This is the program name
PCIINIT21 "pciinit: "

$ This is the product name
PCIINIT24 "PC-Interface "

$ This is printed if pciinit has already been run
PCIINIT25 "giÖ inizializzata"

$ This warns the user to run PCIINIT without the /e option to initialize the
$ software.
PCIINIT26 "Avviso: PC-I non ä stato inizializzato.\n\
Eseguire PCIINIT senza l'opzione /e per inizializzare PC-I.\n"

$ The subnet mask is not in the proper format 
PCIINIT27 "maschera sottorete <%1> non valida\n"

$ The subnet mask is not in the proper format 
PCIINIT28 "maschera sottorete <%1.%2.%3.%4> non valida\n"

$ Netware is already installed
PCIINIT30 "impossibile inizializzare dopo che ä stato inizializzato Netware\n"

$ usage message for pciinit
PCIINIT_U "\
uso: pciinit [/i<host>] [/b<host>] [/s<maschera>] [<directory-seriale>]\n\
       dove <host> ä un nome o un indirizzo IP\n\
       dove <maschera> ä un nome o una maschera di sottorete\n\
       pciinit /e       stampa l'impostazione attuale dell'indirizzo di rete\n\
       pciinit /h o /?  stampa questo messaggio\n"

$ for bridge versions prior to release 3.2
PCIINIT_OLD_BRIDGE "avviso: vecchia versione di bridge.drv\n"

$ This is an error found in the bridge
ERROR_MSG "Errore - "

$ This is used to indicate that a particular host did not respond
GEN_ERROR" non ha risposto. Ritentare? (S o N) : "


DISC_MSG "Sessione di PC-Interface terminata. Eseguire il login per ritentare."
ERR_MSG	"ERRORE DI PARAMETRO!\r\n$"
DRV_MSG	 " unitÖ virtuali e $"
JOBS_MSG " voci della tabella dei lavori\r\n\n$"

$ The copy protection violation message
VIOLATION "Violazione della protezione della copia di PC Interface - SISTEMA DISATTIVATO"


$domain LCC.PCI.DOS.PRINTER
$ The following messages are from printer.c.
$
$ These tokens are the words that the user types to select
$ remote/local printer operation.
PCI_REMOTE_TOKEN "remoto"
PCI_LOCAL_TOKEN "locale"
MRG_REMOTE_TOKEN "unix"
MRG_LOCAL_TOKEN "dos"
AIX_REMOTE_TOKEN "aix"

$ the DOS call to set the printer state failed
PRINTER1 "printer: impossibile impostare lo stato.\n"

$ the printer name given is not in the range allowed or is non-numeric
PRINTER2 "%1: Nome del flusso di stampa non valido.\n"

$ This message says that you can't reset the printer program
$ when you are just setting it
PRINTER3 "%1 non ä valido per l'impostazione del programma di stampante\n"


PRINTER4 "Le opzioni /P e /D sono in conflitto\n"

$ The following 5 thru 8 are the usage message for the multistream
$ version of PC-Interface, and the Merge version.
$ The PCI version uses PRINTER7 instead of PRINTER7_M.
$ The Merge version uses PRINTER7_M instead of PRINTER7.
PRINTER5 "uso:\nprinter\n"
$ %1 is the 'local' token, which is "local" in PCI and "dos" in Merge.
PRINTER6 "printer [LPTn] %1\n"
PRINTER7 "printer [LPTn] {host|unitÖ|-} [prog-stampa|/R] [/X[0|1]] [/T[timeout]]\n"
$ %1 is the 'remote' token, for Merge is "unix" (or "aix", on an AIX machine).
PRINTER7_M "printer [LPTn] %1 [prog-stampa|/R] [/X[0|1]] [/T[timeout]]\n"
PRINTER8 "printer [LPTn] [/P|/D] [/X[0|1]] [/T[timeout]]\n"
PRINTER_HELP "printer /h o /? mostra questo messaggio\n"

$ The following 9 and 10 are the usage message for the non-multistream
$ version of PC-Interface
PRINTER9 "uso: printer {%1}\n"
PRINTER10 "               {%1} [/Tn]\n"

PRINTER11 "Impossibile impostare i parametri della stampante (LPT%1)\n"
PRINTER12 "Impossibile definire remota LPT%1. Probabilmente non ä stato eseguito il login.\n"
PRINTER13 "Questa versione di bridge supporta solo LPT1.\n"
PRINTER14 "printer: impossibile impostare lo stato.\n"
PRINTER15 "printer: impossibile acquisire lo stato.\n"
PRINTER16 "Questa versione di bridge non supporta l'opzione %1\n"
PRINTER22 "LPT%1: "


$ This message prints out which drive is associated with which host
PRINTER23 " (%1: %2)"

$ This message prints out the drive of the host for systems that support
$ only one host
PRINTER24 " (%1:)"

$ the following messages, PRINTER25 through PRINTER33 are used
$ to create an informational display for each printer
$ the format of this message is
$ 	LPT#: [local|remote] , print on exit [on|off], 
$	[no timeout | timeout = # ], [default printer | <printprog>]
PRINTER25 ", stampa all'uscita "
PRINTER26 "sç"
PRINTER27 "no"
PRINTER28 ", timeout = %1"
PRINTER29 ", nessun timeout"
PRINTER30 ",\n      "
PRINTER31 ", "
PRINTER32 "\"%1\"\n"
PRINTER33 ", stampante di default\n"

$ this says whether the printer's state is local or remote
PRINTER34 "Stato attuale della stampante: %1"

PRINTER35 " con timeout di %1 secondi."
PRINTER36 " senza timeout."

$ This message indicates that a particular drive is not connected
PRINTER37 "%1: non ä connesso"

$ This message is self-explanatory.
PRINTER38 "Memoria esaurita.\n"

$domain LCC.PCI.DOS.UDIR
$ The following messages are from udir.c

$ %1 is drive number
UDIR1 " Il volume nell'unitÖ %1 "
UDIR2 "non ha etichetta\n"

$ %1 is the drive label 
UDIR3 "ä %1\n"

$ %1 is the path name
UDIR4 " Directory di %1\n"

$ This is the format of the udir output line
$ %1 is the UNIX side name, %2 is the mapped DOS file name,
$ %3 is the owner's name, %4 is the file attributes
UDIR5 "%1 %2%3%4"

$ This is the message that the file is a directory
UDIR7 "<DIR>    "

$ This completes the output line with date (%1) followed by time (%2)
UDIR9 "%1%2\n"

UDIR14 "\nFile non trovato\n"

$ %1 is the number of files found, %2 is for spacing, %3 is the number
$ of bytes found
UDIR15 "\t%1 File %2%3 byte liberi\n"

$ the drive given is invalid
UDIR17 "Specificazione di unitÖ non valida\n"

$ udir usage message
UDIR18 "\
uso: udir [/a] [letteraunitÖ:][percorso][directory|nomefile]\n\
     udir /h o /?    - stampa questo messaggio\n"


$ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
$ ! 	NOTE - convert.c and getopt.c messages are		!
$ !	 stored on the UNIX side in util			!
$ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

$domain LCC.PCI.DOS.DOSWHAT

DOSWHAT2 "Uso: %1 [/f] [/w] file [file] ...\n\
     %1 /h o /? stampa questo messaggio\n"

$ %1 is the file name
DOSWHAT3 "%1: Non trovato %2\n"
DOSWHAT5 "%1:\n\tDirectory\n"
DOSWHAT7 "%1:\n\tDispositivo\n"
DOSWHAT8 "%1: Impossibile aprire %2\n"
DOSWHAT9 "%1: Impossibile eseguire fstat su %2\n"
DOSWHAT11 "%1:\n\tDispositivo\n"

DOSWHAT14 "Tipo di file indeterminato: provare il comando file di UNIX.\n"

DOSTYPE0 " File binario\n"
DOSTYPE1 " File di testo ascii di DOS\n"
DOSTYPE2 " File di testo ascii di UNIX\n"
DOSTYPE3 " Directory\n"
DOSTYPE4 " Dispositivo\n"

$domain LCC.PCI.DOS.PCIDEBUG
$quote "
$ The following messages are from pcidebug.c
$ %1 is the program name - pcidebug
PCIDEBUG1 "%1: Uso: %1 <host|unitÖ> <[=+-~]listaCan|on|off|close> [...]\n\t\t\t\t\tlistaCan = numCan1[,numCan2[...]]\n\
\t\t %1 /h o /?  stampa questo messaggio\n"

$ %1 is the program name, %2 is the argument
PCIDEBUG2 "%1: Argomento non valido: \"%2\"\n"

$ %1 is the program name, %s is the bit value
PCIDEBUG3 "%1: Il bit %2 ä fuori intervallo ammesso\n"

$ %1 is the program name, %2 is the ioctl value
PCIDEBUG4 "%1: Errore in ioctl %2\n"

$ the next three are tokens 
$ if they are changed, change them in PCIDEBUG1 to agree
PCIDEBUG10 "no"
PCIDEBUG11 "sç"
PCIDEBUG12 "chiudi"


$ The following messages are from vdrive.c
$domain	LCC.PCI.DOS.NLSVD
$quote "

USEAGENODRIVE "Nessuna unitÖ virtuale di PCI ha attualmente eseguito il login.\n"
NODRIVE "Nessuna unitÖ virtuale di PCI ha attualmente eseguito il login.\n"
BADDRIVE "Specificatore di unitÖ non valido - %1\n"
USEAGEBADDRIVE "Specificatore di unitÖ non valido - %1\n"
USEAGENOTADRIVE "L'unitÖ %1 non ä un'unitÖ virtuale.\n"
FATAL "Errore fatale nell'acquisizione del nome host.\n"
CONNECT "%1: ä connesso a %2.\n"

$ SCCSID("@(#)messages	6.1	LCC")	/* Modified: 10/15/90 15:48:13 */
$domain LCC.PCI.DOS.CONVERT
$quote "
$ NOTE: '\n' indicates that a new line will be printed
$ The following messages are from convert.c
CONVERT1 "Specificato SIA maiuscolo SIA minuscolo\n"
CONVERT1A "Specificate opzioni incompatibili\n"
CONVERT2 "Specificato SIA unix2dos SIA dos2unix\n"

$ %1 is the file name on which a read error occured
CONVERT3 "Si ä verificato un errore durante la lettura di %1\n"

$ %1 is the fiel name which cannot be opened
CONVERT4 "Impossibile aprire %1\n"

$ input file %1 and output file %2 are identical
CONVERT5 "I file %1 e %2 sono identici\n"

$ an error was encountered writing file %1
CONVERT6 "Si ä verificato un errore durante la scrittura di %1\n"

CONVERT7 "Tabella di traduzione output non fornita\n"

CONVERT8 "Indicato maiuscolo/minuscolo senza traduzione\n"

$ translation table %1 cannot be opened
CONVERT10 "Impossibile aprire tabella di traduzione %1\n"

CONVERT15 "Tabella di traduzione non valida!\n"
CONVERT17 "Tabella di traduzione di input non fornita\n"
CONVERT21 "Scrittura su output non riuscita!\n"
CONVERT30 "Impossibile allocare spazio per il buffer di traduzione!\n"
CONVERT31 "Tabelle di traduzione non impostate!\n"

$ character %1 was untranslatable with the options used
CONVERT32 "\nCarattere intraducibile alla riga %1\n"

$ unknown error %1 occurred
CONVERT42 "Errore di traduzione sconosciuto: %1\n"

CONVERT45 "Tabelle di traduzione non trovate!\n"
CONVERT46 "Tabella di traduzione non valida!\n"
CONVERT60 "Variabile di ambiente COUNTRY non impostata, viene usato 1\n"

$ code page %1 will be used as no other was specified
CONVERT61 "Variabile di ambiente CODEPAGE non impostata, viene usato %1\n"

CONVERT77 "Tabelle di traduzione di input e di output non fornite\n"
CONVERT80 "Avviso! Carattere a 8 bit\n"
CONVERT86 "Manca memoria per elaborare CONVOPT!\n"
CONVERT90 "Impossibile allocare altra memoria!\n"
CONVERT_B1 "Impossibile generare il migliore singolo con il set di caratteri di default!\n"
CONVERT_S1 "\nInformazioni di traduzione:\n"

$ %1 is the number of glyphs
CONVERT_S2 "Grazie tradotte esattamente:\t\t%1\n"
CONVERT_S3 "Grazie tradotte con pió byte:\t%1\n"
CONVERT_S4 "Grazie tradotte con il default dell'utente:\t%1\n"
CONVERT_S5 "Grazie tradotte con la migliore grazia singola:\t%1\n"
CONVERT_S6 "\nNumero totale di grazie elaborate:\t%1\n"

$ %1 is the number of bytes, %2 is the number of lines in the text file
CONVERT_S7 "\nElaborati %1 byte in %2 righe\n"

$ %1 is the name of the program
CONVERT_M1_D "uso: %1   [/opzioni] ... [input [output]]\n"
CONVERT_M3_D "Le opzioni sono: /u      File maiuscole.\n\
\                /l      File minuscole.\n\
\                /f      Forza (opzione vecchia).\n\
\                /b      Binario (opzione vecchia).\n"
CONVERT_M4_D "\                /7      Emetti un avviso se un carattere usa l'ottavo bit.\n\
\                /x      Diretto. Non tradurre.\n\n\
\                /i tbl  Traduci l'input usando la tabella tbl.\n\
\                /o tbl  Traduci l'output usando la tabella tbl.\n\
\n"
CONVERT_M5_D "\                /c c    Usa c come carattere utente di errore di traduzione.\n\
\                /m      Consenti traduzioni di pió caratteri.\n\
\                /a      Interrompi in caso di errore di traduzione.\n\
\                /s      Usa la traduzione del migliore carattere singolo.\n\
\                /z      Gestisci in modo adatto C-Z (dos2unix/unix2dos).\n\
\n"
CONVERT_M6_D "\                /d      Traduci ritorno carrello come line feed.\n\
\                /p      Traduci line feed come ritorno carrello.\n\
\n"
CONVERT_M7_D "\                /q      Non mostrare i messaggi di avviso.\n\
\                /v      Mostra i messaggi di avviso e le statistiche di traduzione.\n\
\                /h o /? Mostra questo messaggio.\n\
"


CONVERT_M1_U "uso: %1   [-opzioni] ... [input [output]]\n"
CONVERT_M3_U "Le opzioni sono: -u      File maiuscole.\n\
\                -l      File minuscole.\n\
\                -f      Forza (opzione vecchia).\n\
\                -b      Binario (opzione vecchia).\n"
CONVERT_M4_U "\                -7      Emetti un avviso se un carattere usa l'ottavo bit.\n\
\                -x      Diretto. Non tradurre.\n\n\
\                -i tbl  Traduci l'input usando la tabella tbl.\n\
\                -o tbl  Traduci l'output usando la tabella tbl.\n\
\n"
CONVERT_M5_U "\                -c c    Usa c come carattere utente di errore di traduzione.\n\
\                -m      Consenti traduzioni di pió caratteri.\n\
\                -a      Interrompi in caso di errore di traduzione.\n\
\                -s      Usa la traduzione del migliore carattere singolo.\n\
\                -z      Gestisci in modo adatto C-Z (dos2unix/unix2dos).\n\
\n"
CONVERT_M6_U "\                -d      Traduci ritorno carrello come line feed.\n\
\                -p      Traduci line feed come ritorno carrello.\n\
\n"
CONVERT_M7_U "\                -q      Non mostrare i messaggi di avviso.\n\
\                -v      Mostra i messaggi di avviso e le statistiche di traduzione.\n\
\                -h o -? Mostra questo messaggio.\n\
"

$ %1 is the filename, %2 is the options
CONVERT_HELP00_D "uso: %1 [/%2] [filein [fileout]]\n"
CONVERT_HELP00_U "uso: %1 [-%2] [filein [fileout]]\n"
CONVERT_HELP01_D "       %1 /h o /? per informazioni dettagliate\n"
CONVERT_HELP01_U "       %1 -h o -? per informazioni dettagliate\n"
CONVERT_HELP1_D "uso: %1 /%2 [filein [fileout]]\n"
CONVERT_HELP1_U "uso: %1 -%2 [filein [fileout]]\n"

$ The following messages are from getopt.c
$ Do NOT change of the order of %1 and %2!
GETOPT1 "Opzione sconosciuta %1%2\n"
GETOPT2 "Manca l'argomento per l'opzione %1%2\n"
$ SCCSID("@(#)messages	5.1	LCC")	/* Modified: 6/3/91 13:46:54 */
$quote "
$domain LCC.PCI.DOS.HOSTDRV
$ The following messages are from hostdrv.c
$ %1 is the drive letter given
HOSTDRV1 "%1: non ä un'unitÖ virtuale"
HOSTDRV2 "%1 non ä un host connesso"

$domain LCC.PCI.DOS.HOSTOPTN
$ The following messages are from hostoptn.c
HOSTOPTN1 "opzioni di selezione dell'host non valide"
