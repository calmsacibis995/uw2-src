$ SCCSID(@(#)dos.msg	1.5 LCC) Merge 3.2.0 Branch Modified 18:17:50 11/9/94
$ SCCSID(@(#)dos.msg	1.74 LCC) Modified 21:46:23 11/2/94
$ English message file for Merge UNIX utility programs:
$	dosexec (dos)
$	dosopt
$	xdosopt
$	crt
$	tkbd
$	xcrt
$	display
$	romsnap
$	newunix
$	pssnap
$	swkey
$	x_msg
$	UI library
$	xmrgconfig
$	mrgconfig
$	mrgadmin
$	admconvert
$
$quote "
$domain LCC.PCI.UNIX.JANUS

UNKNOWN "Messaggio sconosciuto:\n"
ERROR	"ERRORE:%1 %2 %3 %4 %5 %6\n"
WARNING	"AVVISO:\n"
$	-	dosexec
AOPT_BAD "AOPT_BAD: %1: ERRORE: Uso sbagliato dell'opzione 'a' '%2=%3'\n"
$	-	dosexec
$	-	dosopt
AOPT_TOOMANY "AOPT_TOOMANY: %1: ERRORE: Assegnati troppi dispositivi.\n\
Impossibile continuare.\n"
$	-	crt
$	-	tkbd
BAD_ARGS "BAD_ARGS: %1: ERRORE: Questo programma ha ricevuto parametri non validi.\n"
$	-	dosexec
BAD_IMG "BAD_IMG: %1: ERRORE: Il file immagine '%2' non è del tipo previsto.\n"
$	-	crt
$	-	tkbd
BAD_LCS_SET_TBL "BAD_LCS_SET_TBL: %1: ERRORE: Tabella codici non valida.\n\
\   Impossibile usare la tabella %2 o %3. errno=%4."
$	-	dosopt
BAD_OPT_USAGE "BAD_OPT_USAGE: %1: ERRORE: L'opzione '%2' non è usata correttamente.\n"
$	-	dosexec
BAD_SWD "BAD_SWD: %1: ERRORE: Errore di comunicazione UNIX-DOS.\n"
$	-	dosexec
BOPT_ERR "BOPT_ERR: %1: ERRORE: 'dos +b' non è ammesso.\n"
$	-	dosexec
CD_X "CD_X: %1: ERRORE: 'cd' iniziale sull'unità %2 non riuscito.\n\
Directory che non ha potuto passare a: %3%4\n"
$	-aix	pssnap
$	-	romsnap
CHOWN_ERR "CHOWN_ERR: %1: ERRORE: Impossibile eseguire chown sul file '%2', errno = %3.\n"
$	-	dosexec
CL_TOOLONG "CL_TOOLONG: %1: ERRORE: Riga di comando troppo lunga.\n"
$	-	dosexec
CODEPAGE_NS "CODEPAGE_NS: %1: ERRORE: Traduzione di CODEPAGE DOS '%2' non supportata.\n\
\    (File di CODEPAGE '%3/%4' non trovato. err=%5)\n"
$	-	dosexec
CODESET_NS "CODESET_NS: %1: ERRORE: Traduzione della tabella codici UNIX '%2' non supportata.\n\
\    (File di tabella codici '%3/%4' non trovato. err=%5)\n"
$	-	dosexec
CODESET_USE "\n\
Viene usata la tabella codici '%2' al suo posto.\n"
$	-	dosexec
CONFIG_NF "CONFIG_NF: %1: ERRORE: Impossibile usare il file config.sys %2.\n\
Impossibile aprire in lettura.\n"
$	-	newunix
CONS_ONLY "CONS_ONLY: %1: ERRORE: Eseguibile solo dalla console.\n"
$	-	romsnap
$	-	dosopt
CREATE_ERR "CREATE_ERR: %1: ERRORE: Impossibile creare il file '%2', errno = %3.\n"
$	-	dosopt
CRT_DISPLAY "CRT_DISPLAY: %1: ERRORE: Impossibile usare il display tipo '%2' con 'crt'.\n"
$	-	dosexec
CWD_TOOLONG "CWD_TOOLONG: %1: ERRORE: La directory di lavoro attuale è troppo lunga %2.\
\nMax=%3.\n"
$	-	dosexec
DA_FAIL "DA_FAIL: Impossibile collegare direttamente '%2'. Errore con dispositivo '%3'.\n"
$	-	dosexec
DEV_NA "DEV_NA: %1: ERRORE: Il dispositivo richiesto %2 non è disponibile.\n"
$	-	dosexec
DEV_TOOMANY "DEV_TOMANY: %1: ERRORE: Collegati troppi dispositivi.\n"
$	-	dosopt
DFLTS "* default da %2.\n"
$	-	dosopt
NO_OPTIONS "Nessuna opzione in %1.\n"
$	-	dosexec
DINFO_BYTES "Byte"
DINFO_CGA "CGA"
DINFO_COM1 "COM1"
DINFO_COM2 "COM2"
DINFO_DIRECT "diretto"
DINFO_DOS_DIR "DOS (diretto)"
DINFO_DOSPART "Partizioni DOS:"
DINFO_EGA "EGA"
DINFO_RAM "RAM"
DINFO_EMS "EMS"
DINFO_FDRIVES "Unit… a dischetti:"
DINFO_HERC "HERC"
DINFO_LPT "LPT"
DINFO_MONO "MONO"
DINFO_MOUSE "Mouse"
DINFO_NONE "nessuno"
DINFO_OTHER "ALTRO"
DINFO_PMSG1 "Informazioni dispositivo DOS"
DINFO_PMSG2 "Premere <barra spazio> per continuare"
DINFO_TTY "TTY"
DINFO_UD_SHARED "Condiviso UNIX/DOS:"
DINFO_UNIX_DEF "UNIX (default)"
DINFO_UNKNOWN "sconosciuto"
DINFO_USIZE "dimensione sconosciuta"
DINFO_VGA "VGA"
DINFO_VIDEO "Video"
DINFO_2BUTTON "MS 2 pulsanti"
$	-	display
DISPLAY_USAGE	"Uso: display [nome_dispositivo]\n\
\    Se nome_dispositivo non è specificato, il default è l'output standard.\n"
$	-	dosexec
DMA_NA "DMA_NA: %1: ERRORE: Impossibile allocare canale dma %2\n"
$	-	dosexec
DOSDEV_ERR "DOSDEV_ERR: %1: ERRORE: La riga %2 nel file di definizione del dispositivo\n\
'%3'\n\
è sbagliata:\n\
%4\n"
$	-	dosexec
DOSPART_TWOSAME "%1: Avviso: Si sta tentando di collegare la stessa partizione DOS\n\
come due lettere di unità, %2 e %3, con accesso WRITE. Ciò non è permesso.\n\
Continuo con l'accesso READ-ONLY a queste unità.\n"
$	-	dosexec
DPOPT_ERR "DPOPT_ERR: %1: ERRORE: Impossibile eseguire automaticamente autoexec.bat\
\  sull'unità %2.\n\
(Le opzioni +p e +d%2 non sono ammesse contemporaneamente)\n"
$	-	dosexec
DRIVE_LETTER "DRIVE_LETTER: %1: ERRORE: Lettera di unità %2 non ammessa\n"
$	-	dosexec
EM_FAIL "EM_FAIL: %1: ERRORE: Inizializzazione dell'emulatore non riuscita.\n"
$	-	dosexec
EM_VER "EM_VER: %1: ERRORE: Installata una versione sbagliata dell'emulatore.\n"
$	-	dosexec
ENV_NOTSET "ENV_NOTSET: %1: ERRORE: La variabile d'ambiente richiesta '%2'\
\  non è definita.\n"
$	-	dosexec
ENV_TOOMUCH "ENV_TOOMUCH: %1: ERRORE: L'ambiente è troppo lungo %2. max=%3.\n"
$	-	crt
ERR_SEMAPHORE "ERR_SEMAPHORE: %1: ERRORE: Accesso al semaforo.\n"
$	-	dosexec
EXEC_CMDCOM "EXEC_CMDCOM: %1: ERRORE: Esecuzione di command.com non riuscita.\n"
$	-	dosexec
EXEC_ERR "EXEC_ERR: %1: ERRORE: %2 non installato correttamente.\n"
$	-	dosexec
EXEC_FAILED "EXEC_FAILED: %1: ERRORE: Esecuzione di %2 non riuscita, errno = %3.\n"
$	-	dosexec
EXIT_SIGNAL "EXIT_SIGNAL: %1: ERRORE: Intercettato segnale %2.\n"
$	-	dosexec
FATAL "FATAL: Errore fatale in %1:\n%2\n"
$	-	dosopt
FILE_OPTS_ERR "FILE_OPTS_ERR: %1: ERRORE: Errori nelle operazioni su file.\n"
$	-	dosopt
FN "\nFile: %2\n"
$	-	dosopt
FN_TOOLONG "FN_TOOLONG: %1: ERRORE: Nome di file troppo lungo: %2. max=%3.\n"
$	-	dosexec
FORK_FAIL "FORK_FAIL: %1: ERRORE: Fork interna non riuscita. errno=%2\n\
(Troppi processi in esecuzione o memoria esaurita.)\n"
$
FORK_EXEC_FAIL "FORK_EXEC_FAIL: %1: ERRORE: Fork interna non riuscita. errno=%2\n\
(Troppi processi in esecuzione o memoria esaurita.)\n\
Impossibile eseguire %3\n"
$	-	dosexec
IA_FAIL "IA_FAIL: '%2' non è disponibile.\n\
Il dispositivo '%3' è in uso o ha un permesso non corretto.\n"
$	-	dosexec
$	-	dosopt
ILL_DRV "ILL_DRV: %1: ERRORE: Unità '%2' non ammessa.\n"
$	-	dosexec
$	-	dosopt
ILL_MX "ILL_MX: %1: ERRORE: Selettore di memoria '%2' non ammesso.\n"
$	-	dosexec
$	-	dosopt
ILL_OPT "ILL_OPT: %1: ERRORE: Opzione '%2' non ammessa.\nUsare +h per avere la lista delle opzioni.\n"
$	-	dosexec
IMPROPER_INSTL "IMPROPER_INSTL: %1: ERROR: Non installato correttamente.\n\
\  I permessi o i privilegi non sono corretti.\n"
$	-	dosexec
INITDEV_FAIL "INITDEV_FAIL: %1: ERRORE: Impossibile inizializzare '%2=%3'.\n"
$	-	dosexec
INIT_FAIL "INIT_FAIL: %1: ERRORE: Impossibile inizializzare il dispositivo '%2'.\n"
$	-	dosexec
INTERNAL_ERR "INTERNAL_ERR: %1: ERRORE: Errore interno. %2 non riuscita. %3 %4 %5 %6\n"
$	-	dosexec
MEMORY_ERR "MEMORY_ERR: %1: ERRORE: Memoria esaurita.\n"
$	-	dosexec
INVALID_DEV "INVALID_DEV: %1: ERRORE: %2 non è un dispositivo valido. (%3)\n"
$	-	dosexec
INVALID_TMM "INVALID_TMM: %1: ERRORE: %2 non è un dispositivo valido.\n\
(Intervallo di I/O mappato in memoria %3-%4 non valido)\n"
$	-	dosexec
INVALID_MEMMAP "INVALID_MEMMAP: %1: ERRORE: Collegamento diretto non valido\n\
intervallo di I/O mappato in memoria %2-%3\n"
$	-	dosexec
BAD_PRINTER_CMD "BAD_PRINTER_CMD: Comando stampante non valido per %s.\n"
$	-	crt
$	-	xcrt
IOCTL_FAIL "IOCTL_FAIL: %1: ERRORE: Ioctl %2 su dispositivo %3 non riuscita. Errno=%4.\n"
$	-	dosexec
KILL_FAIL "KILL_FAIL: %1: AVVISO: problemi nell'arresto del processo di schermo.\n"
$	-	dosexec
LOADDEV "LOADDEV: %1: ERRORE: Errore di loaddev.\n"
$	-	dosexec
LOAD_FAIL "LOAD_FAIL: %1: ERRORE: Caricamento del file immagine '%2' non riuscito.\nerrno = %3.\n"
$	-	dosexec
MAX_USER "Impossibile avviare un'altra sessione DOS/Windows, perché il\n\
massimo numero ammesso di utenti DOS/Windows (%2) sta attualmente usando il sistema.\n"
$	-	dosexec
MEM_NA "MEM_NA: %1: ERRORE: Impossibile allocare memoria %2\n"
$	-	dosexec
$	-	dosopt
MEM_OOR "MEM_OOR: %1: ERRORE: Memoria richiesta fuori dai limiti. Max=%2 Min=%3.\n"
$	-	dosexec
MERGE_NA "MERGE_NA: %1: ERRORE: Merge non è configurato nel sistema.\n\
\  Impossibile eseguire DOS.\n"
$	-	dosexec tkbd ...
MERGE_DEV_NA "MERGE_DEV_NA: %1: ERRORE: Impossibile accedere alle chiamate di sistema di Merge.\n"
$	-	newunix
NEWUNIX_USAGE "\nUso: %2 [-n numero_vt] [-p] [[-e] comando]\n"
$	-	dosexec
NO_DOS "NO_DOS: %1: ERRORE: DOS non è installato.\n\
\  Eseguire 'dosinstall' per installare il DOS prima di usare Merge.\n"
$	-	dosexec
NO_PATH "NO_PATH: %1: ERRORE: Il PERCORSO UNIX non è stato impostato.\n"
$	-	dosexec
NO_LICENCE "NO_LICENCE: %1: Nessuna licenza disponibile per eseguire ora DOS/Windows.\n"
$	-	dosexec
NO_USERS "NO_USERS: %1: I servizi DOS/Windows di Merge sono stati chiusi.\n\
Ora non è consentito l'uso di DOS/Windows.\n"
$	-	dosexec
$	-	crt
$	-	tkbd
$	-	dosexec
NOT_DEV "NOT_DEV: %1: ERRORE: %2 (%3) non è un dispositivo.\n"
$	-	dosexec
$	-	dosopt
NOT_DOS_PROG "NOT_DOS_PROG: %1: ERRORE: '%2' non è un programma DOS.\n"
$	-	dosopt
NOT_RW_ERR "NOT_RW_ERR: %1: ERRORE: Impossibile aprire il file '%2'.\n\
\  Il file non esiste o non si è autorizzati alla lettura/scrittura.\n"
$	-	dosopt
NOT_RW_WARN "NOT_RW_WARN: %1: AVVISO Impossibile aprire il file '%2'.\n\
\  Il file non esiste o non si è autorizzati alla lettura/scrittura.\n"
$	-	dosexec
$	-	dosopt
$	-	display
$	-aix	pssnap
$	-	romsnap
NOT_R_ERR "NOT_R_ERR: %1: ERRORE: Impossibile aprire il file '%2'.\n\
\  Il file non esiste o non si è autorizzati alla lettura.\n"
$	-	dosopt
NOT_R_WARN "NOT_R_WARN: %1: AVVISO Impossibile aprire il file '%2'.\n\
\  Il file non esiste o non si è autorizzati alla lettura.\n"
$	-	dosopt
NOT_W_ERR "NOT_W_ERR: %1: ERRORE: Impossibile aprire il file '%2'.\n\
\  Il file non esiste o non si è autorizzati alla scrittura.\n"
$	-	dosexec
NO_BG_CMD "NO_BG_CMD: %1: ERRORE: Impossibile eseguire in background un programma\n\
\          DOS orientato allo schermo. Usare 'dos ...' invece di 'dos ... &'\n"
$	-	dosexec
NO_BG_DOS "NO_BG_DOS: %1: ERRORE: Impossibile ridirigere una sessione DOS\n\
\  o eseguire una sessione DOS in background.\n"
$	-	dosopt
NO_FILENAME "NO_FILENAME: %1: ERRORE: Deve avere un nome di file.\n"
$	-	crt
NO_GRAPHICS "\
\r\n\
\          +============================+ \r\n\
\          |                            | \r\n\
\          |          GRAFICA           | \r\n\
\          |        IMPOSSIBILE         | \r\n\
\          |            SU              | \r\n\
\          |      TERMINALE  ASCII      | \r\n\
\          |                            | \r\n\
\          |  Uscire dal modo grafico   | \r\n\
\          |                            | \r\n\
\          +============================+ \r\n\
\r\n\
\n"
$	-	dosexec
NO_INIT_DATA "NO_INIT_DATA: %1: ERRORE: Impossibile acquisire dati di inizializzazione.\n"
$	-	dosexec
NO_MORE "NO_MORE: %1: ERRORE: Memoria disponibile esaurita. Errno=%2\n"
$	-	dosexec
$	-	newunix
NO_RESOURCES "NO_RESOURCES: %1: ERRORE: Risorse disponibili esaurite.\n"
$	-	newunix
NO_VT "NO_VT: %1: ERRORE: Nessun vt disponibile.\n"
$	-	dosexec
NO_VMPROC "NO_VMPROC: %1: ERRORE: Nessun processo vm.\n"
$	-	crt
NO_WIDE "\
\r\n\
\          +===============================+ \r\n\
\          |                               | \r\n\
\          |        SOLO 80 COLONNE        | \r\n\
\          |              SU               | \r\n\
\          |        TERMINALE ASCII        | \r\n\
\          |                               | \r\n\
\          | Uscire dal modo a 132 colonne | \r\n\
\          |                               | \r\n\
\          +===============================+ \r\n\
\r\n\
\n"
$	-	dosexec
ON_ERR "ON_ERR: %1: ERRORE: DOS orientato allo schermo non eseguibile con ON.\n"
$	-	dosexec
PATH_TOOMUCH "PATH_TOOMUCH: %1: ERRORE: Variabile ambiente PATH troppo lunga %2. max=%3.\n"
$	-	dosexec
PROC_EXIT "PROC_EXIT: %1: ERRORE: Il processo necessario %2 è terminato inaspettatamente.\n\
 Uscita.\n"
$	-	dosexec
$	-	dosopt
$	-aix	pssnap
$	-	romsnap
$	-	tkbd
READ_ERR "READ_ERR: %1: ERRORE: Impossibile leggere il file '%2'.\n"
$	-	dosexec
PORT_NA "PORT_NA: %1: ERRORE: Impossibile allocare porte %2\n"
$	- dosexec (SVR4 only)
REBOOT_FOR_PARTITIONS "\
%1: Avviso:\n\
Le partizioni DOS non saranno accessibili fino a che il sistema viene riavviato.\n"
$	-	dosopt
RMV_OPTS_ERR1 "RMV_OPTS_ERR1: %1: ERRORE: Impossibile rimuovere l'opzione %2.\n\
\  L'opzione non è stata installata."
$	-	dosopt
RMV_OPTS_ERR2 "RMV_OPTS_ERR2: %1: ERRORE: Impossibile rimuovere l'opzione %2.\n\
\  Opzione non consentita.\n"
$	-	romsnap
ROMSNAP_BAD_ADDR "ROMSNAP_BAD_ADDR: %1: ERRORE: Indirizzo iniziale di ROM non corretto %2.\n"
$	-	romsnap
ROMSNAP_USAGE "\nUso: romsnap indirizzo-inizio-rom file-immagine-rom [-k].\n"
$	-	dosopt
$	-aix	pssnap
$	-	romsnap
SEEK_ERR "SEEK_ERR: %1: ERRORE: Problemi nell'uso del file %2. (errore di seek).\n"
$	-	dosexec
SEND_SIG_ERR "SEND_SIG_ERR: %1 Impossibile inviare il segnale %2 a %3, errno = %4.\n"
$	-	dosexec
SERIOUS	"SERIOUS: Grave errore in %1:\n%2\n"
$	-	dosexec
SHAREINIT "SHAREINIT: %1: ERRORE: Impossibile inizializzare segmento di memoria condivisa.\n\
Errore = %2 (%3)\n"
$	-	dosexec
SHELL_NF "SHELL_NF: %1: ERRORE: Impossibile usare SHELL=%2. File non trovato.\n"
$	-	crt
SHMA_FAILED "SHMA_FAILED: %1: ERRORE: Impossibile usare memoria condivisa. errno=%2\n"
$	-	dosexec
SHMC_FAILED "SHMC_FAILED: %1: ERRORE: Impossibile creare memoria condivisa.\n\
Errno=%2 a=%3 s=%4\n"
$	-aix	dosexec
SITE_NI "SITE_NI: %1: ERRORE: Merge non è installato in %2.\n"
$	-	dosexec
$	-	dosopt
SOPT_USAGE "SOPT_USAGE: %1: ERRORE: +s timeout numero fuori intervallo.\n\
Per indicare nessun timeout usare 0.\n\
Per specificare un timeout indicare da %2 a %3.\n"
$	-	dosexec
SWITCHDOS_FAIL "SWITCHDOS_FAIL: %1: ERRORE: Errore di comunicazione UNIX-DOS.\n\
errno=%2.\n"
$	-	dosexec
$	-	crt
SWKEY_FAIL "SWKEY_FAIL: ERRORE: Impossibile eseguire switchkey da un terminale.\n"
$	-	swkey
SWKEY_CUR "\nAttuale sequenza di commutazione di schermo di Merge 386 e Xsight: %2%3%4F<n>.\n"
$	-	swkey
SWKEY_USAGE "\n%1: ERRORE: Opzione non valida\n\
Uso: switchkey [-acs]\n"
$	-	swkey
SWKEY_NEW "\nNuova sequenza di commutazione di schermo di Merge 386 e Xsight: %2%3%4F<n>.\n"
$	-	swkey
TERM_CAP "TERM_CAP: %1: ERRORE: Il terminale non ha le funzioni di supporto\n\
\   per il DOS remoto orientato allo schermo.\n"
$	-	dosexec
TERM_ERR "TERM_ERR: %1: ERRORE: Nessuna terminfo per TERM '%2'.\n"
$	-	dosexec
TOKENPAIR_NF "TOKENPAIR_NF: %1: ERRORE: Impossibile trovare i tipi\n\
di token appropriati per\n'%2=%3'.\n"
$	-	dosexec
TOKEN_BAD "TOKEN_BAD: %1: ERRORE: Token non valido %2 (%3).\n"
$	-	dosexec
TOKEN_NF "TOKEN_NF: %1: ERRORE: Non trovato token '%2' del tipo richiesto.\n"
$	-	dosexec
BAD_DRIVE "BAD_DRIVE: %1: ERRORE: lettera dell'unità sbagliata %2.\n"
$	-	dosexec
LPT_BAD "LPT_BAD: %1: ERRORE: Impossibile impostare LPT2 e LPT3 per un tipo di token a collegamento diretto.\n"
$	-	dosopt
UI_PROB "UI_PROB: %1: ERRORE: Impossibile disinstallare il file '%2'.\n\
\     Il file non è attualmente installato.\n"
$	-	dosexec
UMB_CONFLICT "UMB_CONFLICT: %1: ERRORE: Conflitto nell'assegnazione di blocchi\n\
\     di memoria superiore (UMB) %2-%3\n"
$	-	dosexec
UPOPT_ERR "UPOPT_ERR: %1: ERRORE: Le opzioni +p e +u non sono ammesse\
\  contemporaneamente.\n"
$	-	dosexec
VECT_NA "VECT_NA: %1: ERRORE: Impossibile allocare il vettore di interrupt %2\n"
$	-	crt
VMATTACH_FAILED "VMATTACH_FAILED: %1: ERRORE: Impossibile collegare la memoria vm86.\n\
Errno=%2\n"
$	-	dosexec
VMSVR_FAIL "VMSVR_FAIL: %1: Inizializzazione del server VM non riuscita. errno=%2.\n"
$	-	crt
$	-	dosexec
VM_DIED "VM_DIED: %1: ERRORE: Il processo VM86 è morto.\n"
$	-	dosexec		janus_main.c
WAIT_RET_ERR "WAIT_RET_ERR: %1: ERRORE: ritorno prematuro della chiamata di sistema wait\n\
errno = %2.\n"
$	-	dosopt
$	-aix	pssnap
$	-	romsnap
WRITE_ERR "WRITE_ERR: %1: ERRORE: Impossibile scrivere il file '%2'.\n"
$	-	dosopt
ZOPT_ERR "ZOPT_ERR: %1: ERRORE: Impossibile usare l'opzione 'z' o 'Z'\n\
su un file dei default %2.\n"
$	-	dosopt
ZOPT_EXTRA "ZOPT_EXTRA: %1: ERRORE: È ammessa una sola opzione dopo 'z'.\n"
$	-	dosopt
ZOPT_MISSING "ZOPT_MISSING: %1: ERRORE: Manca l'opzione dopo 'z'.\n"
$	-	dosopt
ZOPT_TOOMANY "ZOPT_TOOMANY: %1: ERRORE: Troppe voci +z. Impossibile continuare.\n"
$	-	dosexec
HELP_EXEC "\n\
\   Uso: %1 [flag...] nomefile arg...\n\
\   Uso: %1 [flag...]\n\
\   La prima forma esegue DOS nomefile; la seconda esegue DOS 'command.com'.\n\
\   I flag modificano le opzioni installate o di default, e danno direttive\n\
\   per modificare le normali operazioni di esecuzione dos.\n\
\   ('+' indica selezione, '-' indica deselezione.)\n\
\     +- a[x] Assegna dispositivo 'x'.\n\
\     +- b    Programma a norme. (ridirige stdio a/da UNIX)\n\
\     +  c    Comando: Il nome di file del comando DOS viene passato senza\n\
\             modifiche al DOS.\n\
\     +  dX   Imposta l'unità attuale iniziale. X= da a fino a z.\n\
\     +- e[f] Usa il file di configurazione del dispositivo 'f'.\n\
\     +  h    Visualizza informazioni di guida (questo schermo). Il DOS non\n\
\             viene eseguito.\n\
\     +- l[f] Carica il file immagine di dos 'f' ('f' può anche essere un nome\n\
\             di directory).\n\
\     +  mn   Dimensione memoria. Numero 'n' (decimale) in Megabyte.\n\
\     +- p[f] Rendi 'Permanente' command.com. 'f' indica quale autoexec.bat\n\
\             eseguire.\n\
\     +- s[n] Fai lo spool dell'output della stampante DOS su stampante UNIX.\n\
\             Il timeout è di 'n' secondi.\n\
\     +- t    Traduci gli argomenti della riga di comando DOS.\n"

$ 	-	dosopt
HELP_INST "\n\
\Uso: %1 [flag...] nomefile...\n\
\   Opzioni di installazione per nomefile. I flag di installazione sono\n\
\   preceduti da '+' o '-' che indicano, rispettivamente, la selezione o\n\
\   la rimozione di quell'opzione. Se non vengono specificate opzioni o\n\
\   direttive, vengono stampate le opzioni attuali.\n\
\     +- a[x] Assegna dispositivo 'x'.\n\
\     +- b    Programma a norme. (ridirige l'I/O standard da/a UNIX)\n\
\     +- dX   Imposta unità iniziale attuale. X= da a fino a z.\n\
\     +- e[f] Usa file di configurazione di dispositivo 'f'.\n\
\     +  h    Schermo di informazioni di guida (questo schermo).\n\
\     +- l[f] Carica file immagine dos 'f' ('f' può anche essere un nome di\n\
\             directory).\n\
\     +  mn   Dimensione memoria. Il numero 'n' (decimale) è in Megabyte.\n\
\     +- p[f] Rendi 'Permanente' command.com. 'f' indica quale autoexec.bat\n\
\             eseguire.\n\
\     +- s[n] Trasferisci l'output della stampante DOS alla stampante UNIX.\n\
\             Il timeout è 'n' secondi.\n\
\     +- t    Traduci gli argomenti di una riga di comando DOS.\n\
\     +- v    Verboso. Mostra il messaggio di riconoscimento.\n\
\     +  y    Rendi i file COM eseguibili da UNIX. Non necessaria quando\n\
\             sono impostate altre opzioni.\n\
\     +  zX   Rimuovi o ripristina l'opzione X.\n\
\     +  Z    Rimuovi tutti i dati di installazione dal file.\n"
$
$ Note: The messages for the ifor_pm_* calls are for SCO only dosexec.
$
$ Error messages for error returns from 'ifor_pm_init_sco()'
$
$ --- For return value:  IFOR_PM_REINIT
PM_INIT_REINIT "\
%1: AVVISO - Tentativo di reinizializzare il policy manager. (IFOR_PM_REINIT)\n"
$
$ --- For return value:  IFOR_PM_BAD_PARAM
PM_INIT_BAD_PARAM "\
%1: ERRORE- Parametro non valido per ifor_pm_init_sco. (IFOR_PM_BAD_PARAM)\n"
$
$ --- For return value:  IFOR_PM_FATAL
PM_INIT_FATAL "\
%1: ERRORE- Errore nell'inizializzare il policy manager. (IFOR_PM_FATAL)\n"
$
$ Error messages for error returns from 'ifor_pm_request_sco()'
$
$ --- For return value:  IFOR_PM_BAD_PARAM
PM_REQ_BAD_PARAM "\
%1: ERRORE- Parametro non valido per ifor_pm_request_sco. (IFOR_PM_BAD_PARAM)\n"
$
$ --- For return value:  IFOR_PM_NO_INIT
PM_REQ_NO_INIT "\
%1: ERRORE- Necessaria la licenza prima di inizializzare il policy manager. \n\
(IFOR_PM_NO_INIT)\n"
$
$ --- For return value:  IFOR_PM_FATAL
PM_REQ_FATAL "\
%1: ERRORE- Errore nella richiesta di licenza dal policy manager. (IFOR_PM_FATAL)\n"
$
$ Error messages for error returns from 'ifor_pm_release()'
$
$ --- For return value:  IFOR_PM_BAD_PARAM
PM_RELEASE_BAD_PARAM "\
%1: AVVISO- Parametro non valido per ifor_pm_release. (IFOR_PM_BAD_PARAM)\n"
$
$ --- For return value:  IFOR_PM_FATAL
PM_RELEASE_FATAL "\
%1: AVVISO- Rilascio di licenza terminato in errore. (IFOR_PM_FATAL)\n"
$
$	- 	xdosopt
XDOSOPT_TITLE "Opzioni DOS"
XDOSOPT_START_BUTTON "Inizio"
XDOSOPT_SAVE_BUTTON "Applica"
XDOSOPT_DEFAULT_BUTTON "Default"
XDOSOPT_HELP_BUTTON "Guida"
XDOSOPT_CANCEL_BUTTON "Annulla"
XDOSOPT_VIDEO_TITLE "Video"
XDOSOPT_VGA_LABEL "VGA"
XDOSOPT_CGA_LABEL "CGA"
XDOSOPT_MDA_LABEL "MDA"
XDOSOPT_HERC_LABEL "Hercules"
XDOSOPT_COM_TITLE "Porte COM"
XDOSOPT_COM1_LABEL "COM1"
XDOSOPT_COM2_LABEL "COM2"
XDOSOPT_EMS_TITLE "EMS"
XDOSOPT_DRIVES_TITLE "Unità"
XDOSOPT_DEFAULT_DRIVE "Nessuno"
XDOSOPT_LPT_TITLE "Porte LPT"
XDOSOPT_LPT_NAME "LPT1"
XDOSOPT_SPOOL_LP_NAME   "UNIX (In spool)"
XDOSOPT_DIRECT_LP0_NAME "DOS (Diretto):lp0"
XDOSOPT_DIRECT_LP1_NAME "DOS (Diretto):lp1"
XDOSOPT_DIRECT_LP2_NAME "DOS (Diretto):lp2"
XDOSOPT_STATUS_DONE_MSG "Fine"
XDOSOPT_QUIT_MSG "Modifiche non confermate.\nUscire comunque?"
XDOSOPT_YES_BUTTON "Sì"
XDOSOPT_NO_BUTTON "No"
XDOSOPT_OK_BUTTON "OK"
XDOSOPT_SAVE_ERR_MSG "Non si ha il permesso di salvare\nmodifiche in quel file."
XDOSOPT_MEM_TITLE "Memoria"
XDOSOPT_XMEM_NAME "Standard"
XDOSOPT_EMEM_NAME "EMS"
XDOSOPT_CONFIG_READ_MSG "Impossibile leggere la configurazione DOS o Windows"
XDOSOPT_MEMORY_MSG	"Memoria esaurita."
XDOSOPT_INTERNAL_ERROR	"Errore interno."
$	- 	x_msg
XMSG_DOSERRTITLE "Errore DOS"
XMSG_OKTITLE "OK"
$	- 	xcrt
XCRT_DOS_TITLE	"DOS"
XCRT_WINDOWS_TITLE "Windows"
XCRT_FILE	"File"
XCRT_FILE_M	"F"
XCRT_ZOOM	"Zoom"
XCRT_ZOOM_M	"Z"
XCRT_REFRESH	"Rigenera"
XCRT_REFRESH_M	"R"
XCRT_EXIT	"Esci"
XCRT_EXIT_M	"s"
XCRT_OPTIONS	"Opzioni"
XCRT_OPTIONS_M	"O"
XCRT_FOCUS	"Fuoco mouse a DOS"
XCRT_FOCUS_M	"F"
XCRT_FONTS	"Font DOS"
XCRT_FONTS_M	"D"
XCRT_KEYS	"Tasti acceleratori"
XCRT_KEYS_M	"T"
XCRT_TUNE	"Regola..."
XCRT_TUNE_M	"R"
XCRT_AUTO	"Automatica"
XCRT_AUTO_M	"A"
XCRT_SMALL	"Piccolo"
XCRT_SMALL_M	"P"
XCRT_MEDIUM	"Medio"
XCRT_MEDIUM_M	"M"
XCRT_TO_DOS	"A Dos/Windows"
XCRT_TO_DOS_M	"W"
XCRT_TO_X	"A Desktop X"
XCRT_TO_X_M	"X"
XCRT_OK		"OK"
XCRT_CANCEL	"Annulla"
XCRT_HELP	"Guida"
XCRT_HELP_M	"G"
XCRT_ON_WINDOW	"Su Finestra"
XCRT_ON_WINDOW_M "F"
XCRT_ON_KEYS	"Su Tasti"
XCRT_ON_KEYS_M	"T"
XCRT_INDEX	"Indice"
XCRT_INDEX_M	"I"
XCRT_ON_VERSION "Su Versione"
XCRT_ON_VERSION_M "V"
XCRT_TUNE_TITLE		"Opzioni"
XCRT_TUNE_COLORMAP	"Mappa colori"
XCRT_TUNE_AUTOZOOM	"Autozoom"
XCRT_TUNE_DEFAULTS	"Default di fabbrica"
XCRT_CLIPBOARD	"Aggiornamento appunti X..."
XCRT_CLIPBOARD_DONE	"Aggiornamento appunti X...fatto."
XCRT_VERSION		"Versione"
XCRT_VERSION_TEXT	"DOS Merge\n%1\nCopyright %2\nLocus Computing Corporation"
XCRT_QUIT_MSG "Uscire da DOS?"
XCRT_NOHELP_MSG "Non è disponibile la guida."
XCRT_YES_BUTTON_TEXT "Sì"
XCRT_NO_BUTTON_TEXT "No"
XCRT_OK_BUTTON_TEXT "OK"
XCRT_NODEV_ERR "Errore: Dispositivo %1 non disponibile."
XCRT_INTR_ERR "Errore: Errore interno di MERGE."
XCRT_VT_ERR "Errore: Impossibile acquisire un nuovo schermo."
XCRT_SERVER_ERR "Errore: Il server non rilascia lo schermo"
XCRT_NOMSE_ERR "Sessione DOS non configurata per un mouse."
XCRT_TMPL1 "%1 è zoomato sullo schermo vt%2."
XCRT_TMPL2 "Premere %1 per accedere al menu dos."
XCRT_MSG1 "Grafica EGA/VGA non disponibile in una finestra."
XCRT_MSG2 "Abbandonare la finestra DOS e riavviare DOS."
XCRT_MSG3 "Selezionare Zoom nel menu Finestra per eseguire la grafica."
XCRT_MSG4 "Zoom non può proseguire."
XCRT_MSG5 "Fuoco non è funzionante."
XCRT_VERSION_ERROR "Il driver dello schermo DOS Merge Windows/X\n\
deve essere aggiornato. Consultare\n\
le note sulla versione, l'amministratore\n\
oppure procedere come segue:\n\
1) avviare il DOS\n\
2) digitare\n\
   WINXCOPY <unità>:<dir windows\\system>\n\
   esempio: WINXCOPY D:\\WINDOWS\\SYSTEM"
XCRT_PICTURE_ERROR "Impossibile visualizzare l'immagine. Provare con\nla risoluzione 800x600 o maggiore e\ncon 256 o più colori."
$	- 	xcrt - Old GUI Messages
XCRT_MENU_TITLE "Menu DOS"
XCRT_UNFOCUS_LABEL "Sfuoco"
XCRT_X_COLORS_LABEL "Colori X"
XCRT_DOS_COLORS_LABEL "Colori DOS"
XCRT_FREEZE_LABEL "Autoblocco sì"
XCRT_UNFREEZE_LABEL "Autoblocco no"
XCRT_UNZOOM_LABEL "Zoom indietro"
XCRT_NORMAL_KEYS_LABEL "Tasti DOS"
XCRT_WINDOW_KEYS_LABEL "Tasti Desktop"
XCRT_OLD_MSG3 "Fare clic su Zoom nel menu DOS per passare in modo grafico."
$	- 	xmrgconfig
GUI_OK			"OK"
GUI_CANCEL		"Annulla"
GUI_HELP		"Guida"
GUI_YES			"Sì"
GUI_NO			"No"
GUI_DELETE		"Cancella"
GUI_MODIFY		"Modifica"
GUI_TITLE 		"DOS Merge"
GUI_AUTOMATIC_LABEL	"Automatica"
GUI_CONFIG_TITLE	"Configurazione DOS e Windows"
GUI_NONE_LABEL		"Nessuno"
GUI_HOME_LABEL		"Home"
GUI_DEVICES_BUTTON_LABEL "Dispositivi"
GUI_OPTIONS_BUTTON_LABEL "Opzioni"
GUI_SAVE_LABEL		"Salva"
GUI_SAVE_AS_LABEL	"Salva come..."
GUI_START_LABEL		"Inizio"
GUI_DRIVES_LABEL	"Unità"
GUI_CONFIGURE_LABEL	"Configura"
GUI_DOS_DRIVE_LABEL	"Unità DOS..."
GUI_UNIX_FILESYS_LABEL	"File system UNIX..."
GUI_COM_PORTS_LABEL	"Porte COM"
GUI_COM1_LABEL		"COM1"
GUI_COM2_LABEL		"COM2"
GUI_LPT_PORTS_LABEL	"Porte LPT"
GUI_LPT1_LABEL		"LPT1"
GUI_LPT2_LABEL		"LPT2"
GUI_LPT3_LABEL		"LPT3"
GUI_TIMEOUT_LABEL	"Timeout"
GUI_DEVICES_LABEL	"Dispositivi"
GUI_DEVICES_STATUS_MSG	"C'è un conflitto tra \n%1 e\n%2."
GUI_MULTI_CONFLICT_MSG  "Rilevati conflitti tra più\ndispositivi."
GUI_MEMORY_LABEL	"Memoria"
GUI_STANDARD_LABEL	"Standard"
GUI_EMS_LABEL		"EMS"
GUI_AUTOEXEC_LABEL	"AUTOEXEC.BAT"
GUI_RUN_SYS_LABEL	"Esegui a livello di sistema"
GUI_PERSONAL_LABEL	"Esegui a livello personale"
GUI_OTHER_LABEL		"Altro"
GUI_BROWSE_LABEL	"Visione..."
GUI_EDIT_LABEL		"Modifica file"
GUI_AUTOEXEC_MNEMONIC	"A"
GUI_SYS_AUTO_LABEL	"AUTOEXEC.BAT a livello di sistema..."
GUI_PERSONAL_AUTO_LABEL "AUTOEXEC.BAT personale..."
GUI_OTHER_AUTO_LABEL	"Altro AUTOEXEC.BAT..."
GUI_CONFIG_LABEL	"CONFIG.SYS"
GUI_CONFIG_MNEMONIC	"C"
GUI_SYS_CONFIG_LABEL	"CONFIG.SYS a livello di sistema..."
GUI_PERSONAL_CONFIG_LABEL "CONFIG.SYS a livello personale..."
GUI_OTHER_CONFIG_LABEL	"Altro CONFIG.SYS..."
GUI_WINDOWS_SIZE_LABEL	"Dimensione di Windows"
GUI_CUSTOM_LABEL	"Personalizzate:"
GUI_RESIZE_LABEL	"Ridimensionamento manuale..."
GUI_WIDTH_LABEL		"Larghezza:"
GUI_HEIGHT_LABEL	"Altezza:"
GUI_DOS_SIZE_LABEL	"Dimensione DOS"
GUI_DOS_FONT_LABEL	"Font DOS"
GUI_SCALE_DOS_LABEL	"Scalatura grafica DOS"
GUI_DISPLAY_TYPE_LABEL	"Tipo di schermo"
GUI_SMALL_LABEL		"Piccolo"
GUI_MEDIUM_LABEL	"Medio"
GUI_LARGE_LABEL		"Grande"
GUI_X1_LABEL		"x1"
GUI_X2_LABEL		"x2"
GUI_MDA_LABEL		"MDA"
GUI_HERCULES_LABEL	"Hercules"
GUI_CGA_LABEL		"CGA"
GUI_VGA_LABEL           "VGA"
GUI_OPTIONS_LABEL	"Opzioni"
GUI_AUTOZOOM_LABEL	"Autozoom"
GUI_COLORMAP_LABEL	"Mappa colori"
GUI_ACCEL_KEYS_LABEL	"Tasti acceleratori per X"
GUI_FACTORY_LABEL	"Default di fabbrica"
GUI_COMMAND_LABEL	"Comando"
GUI_DOS_DRIVE_TITLE	"Unità DOS"
GUI_READ_ONLY_LABEL	"Sola lettura"
GUI_EXCLUSIVE_LABEL	"Esclusivo"
GUI_FILES_LABEL	 	"File"
GUI_FILTER_LABEL	"Filtro"
GUI_DIRECTORIES_LABEL	"Directory"
GUI_SELECTION_LABEL	"Selezione"
GUI_RESIZE_TITLE	"Dimensione finestra Windows"
GUI_FILE_LABEL		"File"
GUI_RESIZE_MESSAGE	"Usare il mouse o la tastiera per ridimensionare questa\nfinestra alla dimensione preferita per Microsoft Windows."
GUI_STATUS_MSG		"Riga di stato"
GUI_CONFIRM_MSG		"Sono stati fatti cambiamenti alla configurazione.\nDevo veramente uscire?"
GUI_FILE_DOESNT_EXIST_MSG "Il file non esiste."
GUI_DIR_DOESNT_EXIST	"La directory non esiste."
GUI_FILE_IS_DIR_MSG	"Il file selezionato è una directory, impossibile aprirla per modificarla."
GUI_PERMISSION_DENIED	"Permesso negato."
GUI_NOT_DIR		"La selezione specificata non è una directory."
GUI_EDIT_ERROR		"Errore nella modifica del file"
GUI_OPEN_CONFIG_ERROR	"Errore nell'apertura del file di configurazione."
GUI_CONFIG_DOESNT_EXIST "La configurazione \"%1\" non esiste."
GUI_INTERNAL_ERROR	"Errore interno"
GUI_MEMORY_ERROR	"Memoria esaurita."
GUI_WRITE_CONFIG_ERROR	"Impossibile salvare il file di configurazione:\n%1"
GUI_DELETE_CONFIG_ERROR	"Impossibile cancellare il file di configurazione."
GUI_ALREADY_EXISTS	"La configurazione \"%1\" esiste già."
GUI_CONFLICT_MSG	"Esistono conflitti hardware tra i seguenti dispositivi:\n%1.\nPer risolvere questi conflitti, deselezionare una o più di\nqueste voci nell'area \"Dispositivi\". Se si vogliono usare\nanche questi dispositivi, bisogna riconfigurarne uno o più\nper evitare conflitti hardware."
GUI_DRIVE_WARNING	"Avviso: \"%1\" non esiste."
GUI_CANT_START		"Impossibile avviare la sessione DOS o Windows."
GUI_NO_DRIVES_ERROR	"Nessuna unità DOS disponibile."
GUI_CANT_READ_FILE	"Impossibile leggere il file \"%1\"."
GUI_CANT_CREATE_FILE	"Impossibile creare il file \"%1\"."
GUI_VIEW_FILE_MSG	"Il file \"%1\" non è accessibile in scrittura. Vuoi vederlo?"
GUI_CREATE_FILE_MSG	"Il file \"%1\" non esiste. Vuoi crearlo?"
$	-	mrgadmin messages
ADM_USAGE_CMD "Uso: mrgadmin class add token:file:nome completo\n     mrgadmin class delete token\n     mrgadmin class list [token]\n     mrgadmin class update token:file:nome completo\n     mrgadmin class printdef token\n"
ADM_CLASS_MSG "Errore: le classi ammesse sono:\n"
ADM_NO_CLASS_MSG "Impossibile ottenere informazioni sulla classe \"%1\".\n"
ADM_NO_CLASS_ENTRIES_MSG "Impossibile trovare voci per la classe \"%1\".\n"
ADM_BAD_TOKEN_MSG "Impossibile ottenere informazioni sul token \"%1\".\n"
ADM_PERMISSION_MSG "Bisogna essere utente root per modificare l'amministrazione di Merge.\n"
ADM_CANT_DELETE_TOKEN_MSG "Impossibile cancellare token \"%1\"\n"
ADM_BAD_TOKEN_DEF_MSG "La definizione del token deve essere del tipo \"token:file:nome\".\n"
ADM_CANT_READ_MSG	"Impossibile leggere \"%1\": %2.\n"
$	-	mrgadmin commands
ADM_ADD_STR		"add"
ADM_DELETE_STR		"delete"
ADM_LIST_STR		"list"
ADM_UPDATE_STR		"update"
ADM_PRINTDEF_STR	"printdef"
$	-	admin library messages
ADM_NO_ERROR "Errore interno - nessun errore."
ADM_INTERNAL_ERROR "Errore interno nella libreria admin."
ADM_PARSE_VARIABLE "Nome di variabile non riconosciuto."
ADM_PARSE_NO_NUMBER "Deve essere specificato un numero."
ADM_PARSE_NUMBER "Valore numerico non ammesso."
ADM_PARSE_ILLEGAL_VARIABLE "Variabile non ammessa per il tipo di collegamento specificato."
ADM_PARSE_MAX_NUMBERS "Superato il numero massimo di voci ammesso."
ADM_MEMORY "Memoria esaurita."
ADM_PARSE_MAX_NUMBER "Superato il valore numerico massimo."
ADM_PARSE_RANGE "Specifica di intervallo non valida."
ADM_BAD_ATTACH "Specifica di tipo di collegamento non valida."
ADM_BAD_DEV_TYPE "Specifica di tipo di dispositivo non valida."
ADM_BAD_FAILURE_ACTION "Specifica di azione in caso di errore non valida."
ADM_BAD_USER_ACCESS "Specifica di accesso dell'utente non valida."
ADM_BAD_VPI_PPI_OPTION "Opzione VPI/PPI non valida."
ADM_BAD_DRIVE_OPTION "Opzione di unità non valida."
ADM_MISSING_VPI_DEV "Manca la prevista specifica di dispositivo VPI."
ADM_MISSING_PPI_DEV "Manca la prevista specifica di dispositivo PPI."
ADM_MISSING_DRIVE_NAME "Manca la prevista specifica di nome di unità."
ADM_MISSING_PRINTER_CMD "Manca la prevista specifica di comando stampante."
ADM_IRQ_MISMATCH "Discordanza tra IRQ e IRQ fisico."
ADM_DMA_MISMATCH "Discordanza tra DMA e DMA fisico."
ADM_IOPORTS_MISMATCH "Discordanza tra porte I/O e porte I/O fisiche."
ADM_MEM_MISMATCH "Discordanza tra I/O mappato in memoria e I/O mappato in memoria fisico."
ADM_SYSERR "Errore di sistema."
ADM_CLASS_NOT_FOUND "Classe non trovata"
ADM_TOKEN_NOT_FOUND "Token non trovato."
ADM_LOCK "Impossibile mettere in lock il file dei token."
ADM_TOKEN_EXISTS "Il token esiste già."
ADM_BAD_TOKEN "I nomi di token sono limitati ai caratteri A-Z, a-z, 0-9 e \"-\"."
ADM_NO_ATTACH "Manca la prevista specifica di tipo di collegamento."
ADM_PARSE_OPTION "Valori di opzione non riconosciuti."
ADM_BAD_UMB "Specifica UMB non ammessa. Deve rientrare nell'intervallo 0xA0000-0xFFFF."
$       -       messaggi admconvert
ADMCVT_CANT_OPEN "Impossibile aprire \"%1\".\n"
ADMCVT_CANT_CREATE "Impossibile creare \"%1\".\n"
ADMCVT_BAD_FORMAT "Errore di formato in /etc/dosdev.\n"
ADMCVT_MEMORY "Memoria esaurita.\n"
ADMCVT_CANT_ADD "Impossibile aggiungere token \"%1\": %2\n"
ADMCVT_LP_CONVERT "Impossibile convertire /usr/lib/merge/lp.\n"
ADMCVT_DOS_DRIVE_NAME "DOS nativo %1:"
ADMCVT_COM1_NAME "IRQ 4 diretto DOS"
ADMCVT_COM2_NAME "IRQ 3 diretto DOS"
ADMCVT_PORT1_NAME "Porta 1 diretta DOS"
ADMCVT_PORT2_NAME "Porta 2 diretta DOS"
$	-	Configuration library errors
CFG_NO_ERROR		"Errore interno - nessun errore."
CFG_PARSE_INTERNAL_ERROR "Errore interno in analisi."
CFG_PARSE_VARIABLE	"Nome di variabile non riconosciuto."
CFG_PARSE_ACCEL		"Valore tasti acceleratori non riconosciuto. Usare \"dos\" o \"x\""
CFG_PARSE_BOOLEAN	"Valore booleano non riconosciuto. Usare \"true\" o \"false\"."
CFG_PARSE_DISPLAY	"Tipo di schermo non riconosciuto. Usare \"auto\", \"vga\", \"cga\", \"mono\" o \"hercules\"."
CFG_PARSE_DRIVE_DEF	"Definizione di unità non riconosciuta. Usare un token di unità o \"nessuno\"."
CFG_PARSE_DRIVE_OPTION	"Opzione di unità non riconosciuta. Usare \"a sola lettura\" o \"esclusiva\"."
CFG_PARSE_DRIVE_LETTER	"Lettera di unità non ammessa. Sono ammesse solo da C a J."
CFG_PARSE_NUMBER	"Valore numerico non ammesso."
CFG_PARSE_SCALE_VALUE	"Valore di scalatua della grafica DOS non riconosciuto. Usare \"auto\", \"1\"," o \"2\"."
CFG_PARSE_NO_NUMBER	"Deve essere specificato un numero."
CFG_PARSE_OPTION	"Valori di opzione non riconosciuti."
CFG_PARSE_FONT		"Valore di font non riconosciuto. Usare \"auto\", \"piccolo\", o \"medio\"."
CFG_UNZOOM_KEY		"Deve essere specificato un nome di tasto X."
OPTCVT_BAD_USER		"Impossibile acquisire informazioni sull'utente attuale."
OPTCVT_BAD_USER2	"Impossibile acquisire informazioni sull'utente %1.\n"
OPTCVT_NOT_DIR		"La directory di configurazione esiste già."
OPTCVT_MKDIR_FAILED	"Impossibile creare la directory di configurazione."
OPTCVT_WRITE_FAILED	"Impossibile scrivere i file di configurazione."
OPTCVT_UNLINK		"Optconvert non ha potuto rimuovere i file di configurazione."
OPTCVT_ADM_ERROR	"Errore di amministrazione durante la conversione."
OPTCVT_BAD_OPTIONS	"Uso: optconvert [-c utente]\n"
OPTCVT_NO_CONVERT	"La configurazione esiste già per l'utente %1.\n"
OPTCVT_BAD_LOG		"Impossibile scrivere sul log %1.\n"
OPTCVT_BAD_CONVERT	"Impossibile convertire +a%1.\n"
CFG_BAD_DIR		"Impossibile aprire la directory di configurazione."
CFG_MEMORY		"Memoria esaurita."
CFG_SYSERR		"Errore di sistema."
CFG_BAD_NAME		"Nome di configurazione non ammesso."
CFG_NOT_DEL	"Non è possibile cancellare le configurazioni \"dos\" o \"win\"."
CFG_HEADER1	"Questo file è stato generato con le utility di configurazione di Merge."
CFG_HEADER2	"Usare le utility di configurazione per modificare questo file."
$	-	mrgconfig
CFG_BAD_CMD	"Errore interno - comando non riconosciuto.\n"
CFG_YES_RESP	"y"
CFG_NO_RESP	"n"
CFG_CONFIRM	"Devo veramente cancellare \"%1\"?"
CFG_RESPONSE	"Rispondi \"%1\" o \"%2\"\n"
CFG_CANT_DELETE "Impossibile cancellare \"%1\".\n"
CFG_DELETED	"Configurazione \"%1\" cancellata.\n"
CFG_NOT_DELETED	"Configurazione \"%1\" non cancellata.\n"
CFG_ADD_USAGE_CMD "Uso: addmrgconfig config-vecchia [copyto] config-nuova.\n"
CFG_DEL_USAGE_CMD "Uso: delmrgconfig config.\n"
CFG_LIST_USAGE_CMD "Uso: listmrgconfig\n"
CFG_DEL_OPT_ERROR "È possibile cancellare solo le opzioni customdev.\n"
CFG_SET_OPT_ERROR "Si può specificare una sola opzione.\"
CFG_USAGE_CMD	"Uso: mrgconfig config list opzione[,opzione...].\n     mrgconfig config set opzione\n     mrgconfig config delete opzione\n"
CFG_COPYTO_STR	"copyto"
CFG_LIST_STR	"list"
CFG_SET_STR	"set"
CFG_DELETE_STR	"delete"
CFG_ALREADY_EXISTS "La configurazione \"%1\" esiste già.\n"
CFG_BAD_OPTION	"Opzione non riconosciuta - \"%1\".\n"
CFG_WRITE_ERROR	"Errore interno - scrittura non riuscita.\n"
CFG_DEL_VALUE_ERROR "Deve essere specificato un valore customdev.\n"
CFG_DEL_NOT_FOUND "Il dispositivo personalizzato \"%1\" non esiste nella configurazione \"%2\".\n"
CFG_READ_ERROR  "Errore interno - impossibile leggere la configurazione.\n"
CFG_LIST_ERROR	"Impossibile acquisire una lista di configurazioni.\n"
CFG_NOLIST_MSG	"Nessuna configurazione da elencare.\n"
$ SCCSID(@(#)install.msg	1.23 LCC) Modified 16:45:28 9/7/94
$ Messages for the install and remove scripts.
$domain LCC.MERGE.UNIX.INSTALL
$quote "

GEN_ERR_1 "L'installazione non può essere completata a causa di un errore.\n"
GEN_ERR_2 "Esaminare %1 per l'output degli errori.\n"
GEN_ERR_3 "Pulizia finale e uscita...\n"
GEN_ERR_4 "Errore nella rimozione.\n"
GEN_ERR_5 "Continuo ....\n"
GEN_ERR_6 "Non viene ricostruito il kernel.\n"

BUILD_FAIL "Costruzione del kernel non riuscita.\n"
LU_FAIL   "link_unix non riuscita\n"
IDB_FAIL  "idbuild non riuscita\n"

REPL_ERR  "Impossibile sostituire %1 attualmente installato.\n\
Rimuovere %2 prima di reinstallare.\n"

I_WARN_1   "Nel kernel in esecuzione è già installato %1.\n"
I_ERR_1   "%1 non può essere installato su questo sistema.\n"
I_ERR_2   "Il pacchetto base Unix deve contenere il supporto\n\
per %1, oppure occorre installare prima\n\
un aggiornamento del pacchetto base Unix.\n"

KERNEL_HAS_MERGE "\n\
Nel kernel di UNIX attualmente in esecuzione è installato %1.\n\
%2 deve essere rimosso completamente prima della reinstallazione,\n\
altrimenti l'installazione può fallire e il kernel potrebbe non\n\
essere collegato appropriatamente.\n"

NO_STREAMS "\n\
Driver streams non installato. Il driver di streams deve essere\n\
installato per potere poi installare %1.\n"

CANT_INST  "Impossibile installare %1.\n"
CANNOT_REMOVE "Impossibile rimuovere %1.\n"
NOSPACE    "\       Spazio insufficiente per ricostruire il kernel.\n"

INSTALL_1 "Installazione dei file %1. Attendere...\n"
INSTALL_2 "Configurazione dei file %1. Attendere...\n"

REMOVE_3 "Rimozione dei file %1. Attendere...\n"
REMOVE_4 "I file della distribuzione %1 sono stati disinstallati.\n"

IDIN_FAIL "idinstall %1 non riuscita.\n" \
ALREADY_INSTALLED "Avviso: Driver %1 già installato.\n"
IDCHK_FAIL  "idcheck -p di %1 ha restituito %2\n"

LINK_FAIL_1 "collegamento di un file non riuscito.\n"
LINK_FAIL_2 "Collegamento non riuscito: %1 %2\n"
CPIO_CP_FAIL "Copia dei file con cpio non riuscita.\n"
IDTUNE_FAIL "idtune non riuscita\n"

$ The following five are used in install and remove when the user is
$ prompted if a re-link of the kernel is wanted.  YESES and NOES are
$ each a list of the acceptable single character responses for yes and no.
YESES "sS"
NOES "nN"
LINK_NOW_Q "Ricostruire il kernel ora? (s/n) "
Y_OR_NO "Rispondere \"s\" o \"n\": "
REBUILD_WARN "Occorre ricostruire il kernel perché %1 funzioni correttamente.\n"
REBUILD_NEED "Occorre ricostruire il kernel perché il sistema funzioni correttamente.\n"

$ This section (to the line with END-PKGADD) is PKGADD/SVR4 specific.

INST_LOG_HDR "\
# Questo è un file di log creato dal processo di installazione\n\
# %1. Eventuali errori durante la %2 parte controllata dell'installazione\n\
# verranno segnalati qui. Questo file viene creato durante l'installazione\n\
# e può essere cancellato dall'utente. Può anche contenere output non di\n\
# errore da altri programmi UNIX (standard).\n"

REM_LOG_HDR "\
# Questo è un file di log creato dal processo di rimozione %1.\n\
# Eventuali errori durante la %2 parte controllata della rimozione verranno\n\
# segnalati qui. Questo file viene creato durante l'installazione e può essere\n\
# cancellato dall'utente. Può anche contenere output non di errore\n\
# da altri programmi UNIX (standard).\n"

INST_NO_VERS "\
ERRORE: Impossibile trovare file Version\n\
       Impossibile installare Merge\n"

PO_ERR_MSG1 "Impossibile creare una voce (%1) per la stampante.\n"

INST_USER_STARTUP "Aggiornamento dei file di inizializzazione utente UNIX ...\n"
INST_PRINTER "Installazione interfaccia stampante DOS ...\n"
INST_CONFIG_SYS "Aggiornamento config.sys globale...\n"
INST_AUTOEXEC "Aggiornamento autoexec.bat globale...\n"
INST_DRV_MSG "Installazione dei driver %1 ...\n"
INST_MOD_MSG "\tmodulo: %1\n"
INST_MERGE "Accensione del driver MERGE ...\n"
INST_DOSX "Accensione del driver DOSX ...\n"
INST_SEG_WRAP "Accensione del driver SEG WRAP ...\n"
INST_DONE "L'installazione di %1 è completa.\n"
INST_CANCEL "Installazione annullata dall'utente.\n"
INST_DRV_SH "Errore nell'installazione dei driver di Merge.\n"
INST_MRG_SH "Problemi nell'installazione dei file di Merge.\n"
INST_X_SH "Problemi nell'installazione dei file Merge X.\n"

REM_USER_STARTUP "Aggiornamento dei file di inizializzazione utente UNIX ...\n"
REM_DOS_FILES "Rimozione dei file DOS (da dosinstall) ...\n"
REM_PRINTER "Rimozione interfaccia stampante DOS ...\n"
REM_PRINTER_NOT "L'interfaccia stampante DOS non viene rimossa.\n"
MISSING_PROG "Manca il programma %1.\n"
REM_CONFIG_SYS "Aggiornamento config.sys globale...\n"
REM_AUTOEXEC "Aggiornamento autoexec.bat globale...\n"
REM_DRIVERS "Rimozione dei driver %1 ...\n"
REM_MERGE "Spegnimento del driver MERGE ...\n"
REM_DOSX "Spegnimento del driver DOSX ...\n"
REM_SEG_WRAP "Spegnimento del driver SEG WRAP ...\n"
REM_DONE "Rimozione di %1 completata.\n"
REM_CANCEL "Rimozione annullata dall'utente.\n"

REM_PROB_MODLIST "Non trovato %1/modlist, nessun modulo rimosso.\n"
REM_PROB_MODULE "Impossibile rimuovere %1.\n"

# request script
REQ_PREV_INSTALL "\n\
$MERGENAME è già installato.\n\
Prima di reinstallarlo, deve essere rimosso.\n\n"

$ END-PKGADD
$ SCCSID(@(#)scripts.msg	1.2 LCC) Merge 3.2.0 Branch Modified 12:38:24 10/14/94
$ SCCSID(@(#)scripts.msg	1.40 LCC) Modified 23:55:19 10/10/94
$ English message file for the Merge scripts.
$quote "

$domain LCC.MERGE.UNIX.SET_NLS
$ Messages for the "set_nls" script.
WARN_PREFIX "AVVISO: "
ERR_PREFIX "ERRORE: "

$domain LCC.MERGE.UNIX.MKVFLP
$ Messages for the "mkvfloppy" script.
USAGE "USO: %1 percorso-completo-nomefile [-s]\n"

$domain LCC.MERGE.UNIX.DOSBOOT
$ Messages for the "dosboot" script.
ERR1 "ERRORE: Non si può usare l'opzione \"+l\" con %1.\n"

$domain LCC.MERGE.UNIX.INITDPART
$ Messages for the "initdospart" script.

INSERT "Inserire un dischetto ad alta densità nell'unità A: e premere <INVIO>: "
FORMAT "Formattare la partizione DOS del disco.\n"
ERR_1  "ERRORE: Non esiste la partizione DOS sul disco.\n"
ERR_2  "ERRORE: Solo l'utente root può formattare la partizione DOS.\n"
ERR_3  "ERRORE: Impossibile formattare il dischetto.\n\
\n\
Controllare che non sia protetto da scrittura.\n"

ERR_4  "ERRORE: Il dischetto sembra guasto.\n"

TEXT_1 "Per poter usare la partizione DOS, occorre prima formattarla.\n\
\n\
La formattazione della partizione DOS non è richiesta, se è già\n\
stata effettuata in precedenza, a meno che non sia stata\n\
modificata la posizione o la dimensione della partizione DOS\n\
dopo la formattazione.\n\
\n\
Avviso: La formattazione della partizione DOS cancella tutti\n\
\       i dati che essa contiene.\n\
Avviso: La formattazione della partizione DOS richiede lo\n\
\       spegnimento del sistema e il riavviamento da\n\
\       dischetto.\n\
\n\
È necessario disporre di un dischetto ad alta densità,\n\
facendo attenzione che non sia protetto da scrittura. Questo\n\
dischetto verrà formattato e perparato per il riavviamento.\n"

$ NOTE: the CONTINUE, YESES, NOES and Y_OR_N string are closely related.
$ The CONTINUE and Y_OR_N are prompts for a yes or no answer.
$ They don't have newline chars, so the cursor will stay at the end
$ of the printed line.
$ The YESSES and NOES strings define the recognized single character
$ yes or no answers.
$ The YESES  string is a list of single characters that mean "yes".
$ The NOES  string is a list of single characters that mean "no".
CONTINUE "Continuare (s/n)? "
Y_OR_N "Rispondere \"s\" o \"n\": "
YESES "sS"
NOES  "nN"

RETRY "Verificare che il dischetto usato sia ad alta densità e privo di difetti.\n\
\n\
Riprovare (s/n)? "

REBOOT "\n\
Per formattare la partizione DOS, è necessario avviare il sistema\n\
dal dischetto che si trova attualmente nell'unità.\n\
\n\
Pertanto, NON rimuovere il dischetto\n\
\n\
Premere <INVIO> per spegnere il sistema per il riavviamento. "

EXIT   "Non viene formattata la partizione DOS.\n"
EXIT_2 "La partizione DOS va formattata prima dell'uso.\n"


$domain LCC.MERGE.UNIX.DOSINSTALL
$ Messages for the dosinstall and dosremove scripts.
$    (sourcefiles: unix/janus/dosinst.sh, unix/janus/dosremove.sh)
EXITING "Uscita...\n"
ERR_0 "Impossibile installare il DOS.\n"
ERR_1 "Solo l'utente root o su può installare il DOS.\n"
ERR_2 "Il DOS è già installato. Prima di poter eseguire la\n\
reinstallazione deve essere rimosso il DOS attualmente installato.\n"
CANNOT_CONTINUE "Impossibile continuare.\n"
$ The BAD_FLOP_NAME error can happen when the user used the "DRIVE_DEV"
$ variable to specify which floppy device to use, and the device does
$ not exist.  If the user did not set DRIVE_DEV, then the internal
$ logic that determines the device filename did not work.  To work around
$ this problem, the use should set DRIVE_DEV to the device name to use.
BAD_FLOP_NAME "Nome di dispositivo dell'unità a dischetti '%1' non valido\n"

RM_MSG "Rimozione del DOS\n"
ERR_RM "Problemi nella rimozione del DOS.\n\
L'installazione del DOS potrebbe non riuscire.\n"
RE_INST "Reinstallazione del DOS\n"
INSTALLING_DOS "Installazione del DOS\n"
INSTALLING_DOSVER "Installazione di %1\n"
DOS_INST_DONE "Installazione del DOS completata.\n"

DSK_DONE "Ora è possibile togliere il dischetto.\n"

DOSINST_MKIMG_FAILED "Mancata creazione di alcuni file immagine del DOS.\n\
\     Installazione DOS parzialmente fallita.\n"

DOSINST_MKIMG_MONO_FAILED "Impossibile creare file immagine mono del DOS.\n\
\     Installazione del DOS interrotta.\n"

NO_SPACE    "     Spazio insufficiente.\n"
NO_SPACE_IN "     Spazio insufficiente in %1.\n"
NEED_SPACE  "     Richiesti: %1 blocchi, Disponibili: %2.\n"

DOSINST_CANT "Impossibile installare %1\n"

DOSINST_DOSBRAND_MENU1 "Scegliere quale DOS installare.\n"

DOSINST_DOSBRAND_MENU2 "\
\      0: Nessuno. Interrompere installazione del DOS.\n\
\n\
Digitare il numero dell'opzione e premere <Invio>: "

DOSINST_01 "Immettere 0 o 1\n"
DOSINST_012 "Immettere 0, 1 o 2\n"
DOSINST_0123 "Immettere 0, 1, 2, o 3\n"
DOSINST_01234 "Immettere 0, 1, 2, 3 o 4\n"
DOSINST_012345 "Immettere 0, 1, 2, 3, 4 o 5\n"
DOSINST_0123456 "Immettere 0, 1, 2, 3, 4, 5 o 6\n"
DOSINST_01234567 "Immettere 0, 1, 2, 3, 4, 5, 6 o 7\n"

DOSINST_SCO_DISKSIZE "\
Immettere la dimensione dei dischi DOS.\n\
\n\
\      1: 3.5\" a bassa densità.\n\
\      2: 3.5\" ad alta densità.\n\
\      3: 3.5\" densità sconosciuta.\n\
\      4: 5.25\" a bassa densità.\n\
\      5: 5.25\" ad alta densità.\n\
\      6: 5.25\" densità sconosciuta.\n\
\      0: Nessuna. Interrompi installazione del DOS.\n\
\n\
Digitare il numero dell'opzione e premere <Invio>: "

DOSINST_FLOP_P "\
Qual'è l'unità a dischetti usata per l'installazione?\n\
\n\
\      1: La prima unità.   (unità '%1')\n\
\      2: La seconda unità. (unità '%2')\n\
\      0: Stop. Interrompi installazione del DOS.\n\
\n\
Digitare il numero dell'opzione e premere <Invio>: "

DOSINST_MSG1 "Installazione di %1 da %2 dischi.\n"

DOSINST_FROM "Installazione dall'unità %1.\n"

WRONG_DISK_1 "\
\      Questo disco non sembra essere quello richiesto.\n\
\      Verificare che il disco corretto si trovi nell'unità opportuna.\n"

WRONG_DISK_N "\
\      Questo disco è già stato letto come disco #%1.\n\
\      Verificare che il disco corretto si trovi nell'unità opportuna.\n"

DOSINST_DW_P "\
Che fare ora:\n\
\      0: Interrompere installazione del DOS.\n\
\      1: Riprovare.\n\
Digitare 0 o 1 e premere <Invio>: "

DOSINST_DW_R "Immettere 0 o 1\n"

DOSINST_NOTINST "Non viene installato il DOS.\n"
DOSINST_AUTO_NA "\
Impossibile installare automaticamente il DOS.\n\
Per installare il DOS dai dischetti DOS\n\
eseguire il programma 'dosinstall'.\n"

CHECK_DISK  "Verifica del disco."
INSERT_S "Inserire il disco DOS %1 nell'unità e premere INVIO. "
INSERT_D "Inserire il disco DOS %1 nell'unità %2 e premere INVIO. "
INSERT_NEXT "Sostituire il disco %1 con il disco %2 e premere INVIO. "
INST_CANCEL "Installazione annullata dall'utente.\n"
INST_SYS "Installazione dei file di sistema del DOS.\n"
READING_MESSAGE "Lettura del dischetto."
CANNOT_CREATE "Errore: Impossibile creare %1\n"
NO_FILE "Errore: Manca %1\n"
MISSING_DIR "Errore: Manca la directory %1\n"
CREATING_DIR "Avviso: Directory %1 mancante \n\
Viene creata.\n"

$ Note: Q_REMOVE, Q_CONTINUE, and Q_ANYWAY don't end in a newline.
$ Also, the answer to them must be either the string in ANS_Y or ANS_N or ANS_Q.
Q_REMOVE "Rimuovere ora il DOS attualmente installato (s/n/u) ? "
Q_CONTINUE "Proseguire con l'installazione del DOS (s/n/u) ? "
Q_ANYWAY "Tentare comunque la rimozione del DOS (s/n/u) ? "
Q_PROMPT "Rispondere s oppure n oppure u\n"
ANS_Y "y"
ANS_N "n"
$ Note: ANS_Q is what users can type at all prompts to "quit" or abort
$ the installation, (also entering 0 does the same thing as ANS_Q).
ANS_Q "q"
DOSINST_0Q_MSG "\
Rispondendo 0 o u a qualsiasi richiesta si termina l'installazione del DOS.\n"
CANCEL_MSG "Annullamento della reinstallazione del DOS.\n"
IMPROPER "Il DOS non è installato appropriatamente.\n"
RM_ABORT "Rimozione del DOS interrotta. Nessun file rimosso.\n"
RM_PROB "Errore: Problemi nella rimozione del DOS.\n"
RM_PROB_1 "Manca elenco file. Impossibile disinstallare completamente il DOS.\n"
RM_ALMOST "Effettuato tentativo di rimozione del DOS.\n"
RM_DONE "Effettuata rimozione del DOS.\n"
BAD_DISK "\
\      Problemi di lettura del disco.\n\
\      Verificare che il disco sia inserito nell'unità corretta.\n"
CLEAN_UP "\
Per completare l'installazione del DOS, liberare spazio sul disco e riavviare.\n"
VDISK_ERR "Errore con disco virtuale. Impossibile installare il DOS.\n"
MISSING_FILE "Errore: %1 non trovato.\n"
INTERNAL_ERR "Errore interno %1 di dosinstall\n"
EXPANDING "Espansione dei file del DOS.\n"
CONFIGURING "Configurazione dei file del DOS.\n"
DRIVEA_NAME "A"
DRIVEB_NAME "B"
DOSINST_NDISKS_Q "Numero di dischetti nel set di %1: "
DOSINST_0_9 "Immettere un numero da 0 a 9\n"
DOSINST_0_ABORT "Immettendo 0 si annulla l'installazione del DOS.\n"
FROM_BUILTIN "(dai file incorporati)"
FROM_FLOP "(dai dischetti)"
MIN_SYSTEM "sistema minimo"
DOSINST_PLS_ENTER "Immettere %1\n"
MISSING_SET "File 'set' mancanti\n"
CREATE_BOOT_FAIL "\
Avviso: Non è stato creato il nuovo file boot.dsk.\n\
\        Utilizzato file boot.dsk esistente.\n"
BOOT_TOOSMALL "\
Avviso: Impossibile creare un disco virtuale abbastanza\n\
\        grande per inizializzare tutte le funzioni\n\
\        DOS NLS, che vengono quindi disattivate.\n"

$domain LCC.MERGE.UNIX.CHECKMERGE
$ Messages for the "checkmerge" script.
USAGE "Uso: %1\n"
MSG_1 "Confronto delle somme di controllo dei file per\n"
MSG_2 "con i valori contenuti in %1...\n"
MSG_3 "è stato modificato dopo l'ultima installazione di Merge.\n"
MSG_4 "Impossibile raccogliere le somme di controllo di %1.\n\
Continua...\n"
MSG_5 "Verifiche di file eseguite.\n"

$domain LCC.MERGE.UNIX.LPINSTALL
$ Messages for the lpinstall script.
MAIN_MENU "\n\
\n\
\n\
\         Programma Installazione stampante Merge\n\
\         ----------------------------------\n\
\n\
\         1) Installa una stampante\n\
\         2) Rimuovi una stampante\n\
\         3) Elenca stampanti\n\
\n\
\         %1) Uscita\n\
\n\
\         ----------------------------------\n\
\         Immettere l'opzione: "
$ Note: QUIT_CHAR must be a single character that mean quit.  It is used
$ as the %1 in the MAIN_MENU message and is compared with what the user
$ typed as the response to the "enter option" question.
QUIT_CHAR "q"
INSTALL "\n\
\         INSTALLA\n\
\         ---------------------\n"
PRINTER "        Nome stampante [%1]: "
MODEL   "        Modello stampante [%1]: "
DEVICE  "        Dispositivo [%1]: "
$ Note: YES_CHAR must be a single character that means yes.  It is used
$ as the %1 in the OK_INST and  %2 in OK_REMOVE messages and is compared
$ with what the user typed as the response to those messages.
YES_CHAR "y"
OK_INST  "        Confermare l'installazione? [%1]: "
OK_REMOVE "        Confermare la rimozione di %1? [%2]: "
CONTINUE "        Premere <INVIO> per continuare"
REMOVE "\n\
\         RIMOZIONE\n\
\         ---------------------\n
\         Nome stampante: "
LIST "\n\
Numero di stampanti installate: %1\n\
--------------------------------------\n"
MISSING_PROG	"Manca il programma %1.\n"
NO_LP		"Il servizio di stampa LP potrebbe non essere installato.\n"
CANNOT_CONTINUE	"Impossibile continuare.\n"

$domain LCC.MERGE.UNIX.MKIMG
$ Messages for the "mkimg" script.

CANNOT_CONTINUE	"Impossibile continuare.\n"
CANNOT_COPY	"Avviso: Impossibile copiare file %1.\n"
CANNOT_MAKE	"Impossibile generare l'immagine %1.\n"
CANNOT_MKDIR	"Impossibile creare la directory %1\n"
CANNOT_READ	"Impossibile leggere il file %1\n"
CANNOT_WRITE	"Impossibile scrivere %1\n"
CONFIG_MSG	"Configurazione dei file immagine DOS.\n"
COPY_PROB	"Problemi nella copia di file sul disco virtuale %1\n"
CREATE_NOROOM	"Spazio insufficiente per creare il file %1\n"
DOS_NI		"Il DOS non è installato\n"
FORMAT_PROB	"Problemi nella formattazione del disco virtuale %1\n"
IMG_MADE	"Generata immagine %1.\n"
INCOMPATABLE	"(Scheda %1 incompatibile?)\n"
MAKING		"Generazione immagine %1\n"
MISSING		"Manca il file %1\n"
MUST_SU		"Solo l'utente root o su può generare l'immagine %1.\n"
NATIVE_MUST_SU	"Solo l'utente root o su può generare l'immagine %1 nativa.\n"
NATIVE_TOKEN	"nativa"
NOT_MADE	"Immagine %1 non generata.\n"
NO_DIRECTORY	"La directory %1 non esiste.\n"
NO_IMG_MADE	"Non sono stati generati i file immagine.\n"
NO_PERMIS_TMP	"Manca il permesso per creare il file temporaneo %1\n"
NO_PERMIS_WRITE	"Manca il permesso per scrivere su %1\n"
NO_ROOM		"(Spazio esaurito?)\n"
NO_ROOM_TMP	"Spazio insufficiente per creare il file temporaneo %1\n"
OPT_NA		"Opzione %1 non ammessa.\n"
REMOVE_FAIL	"Impossibile rimuovere il file %1\n"
SOME_NOT_MADE	"Alcuni file immagine non sono stati generati.\n"
USAGE_LINE	"USO: %1 [cga] [mda] [ d directory ] [devicelow] [+aXXX]\n"
USING_INSTEAD	"Usato %1 in sostituzione.\n"
USING_ONCARD	"Usata ROM %1 sulla scheda video.\n"
WRONG_DISP	"Impossibile creare immagine %1 su display %2.\n"

$domain LCC.MERGE.UNIX.S55MERGE
$ Messages for "S55merge" script.
NOT_DOSINST	"Avviso: Il DOS non viene installato automaticamente.\n"
NOT_MKIMG	"Avviso: I file immagine DOS non vengono creati automaticamente.\n"
DOING_MKIMG	"Creazione automatica in background dei file immagine DOS.\n"

$domain LCC.MERGE.UNIX.PART_SET
$ Messages for the "part_set" script.
BAD_DOSDEV	"File '%1' danneggiato.\n"
MISSING_LINE	"Manca la riga '%1'.\n"

$domain LCC.MERGE.UNIX.MERGEFONTMAKE
$ Messages for the "mergefontmake" script (part of X-Clients package).
MFM_USAGE	"\
USO: mergefontmake 0|1|2|3|4 [s|ML]\n\
Ricompila i font di Merge.\n\
\n\
I font possono essere compilati in quattro modi diversi e\n\
funzionano solo quando sono compilati secondo quanto\n\
si aspetta il server X.\n\
Il primo parametro specifica in quale dei quattro modi compilare\n\
i font: 1, 2, 3 o 4.\n\
Il parametro '0' produce l'uso dei default, qualunque essi siano.\n\
\n\
'mergefontmake' tenta di stabilire quale versione del compilatore\n\
di font si sta usando, dato che le diverse versioni X usano opzioni\n\
diverse per compilare i font. (Alcune versioni usano l'opzione\n\
'-s' invece di '-M' e '-L'). Per imporre il tipo di opzioni usate,\n\
specificare 's' o 'ML' come secondo parametro.\n"
MFM_MISSING	"Errore: Manca %1\n"
MFM_BAD_SECONDP	"Errore: Il secondo parametro deve essere 's' o 'ML' oppure non va specificato.\n"
MFM_NO_XBIN	"Errore: Impossibile trovare la directory bin di X.\n"
MFM_NO_FONTDIR	"Errore: Impossibile determinare la directory dei font.\n"
MFM_I_NOPERMIS	"Errore: Manca il permesso di installare font in %1\n"
MFM_01234	"Errore: Deve essere 0, 1, 2, 3 o 4.\n"
MFM_U_NOPERMIS	"Errore: Manca il permesso di aggiornare %1\n"
MFM_FC_ERR	"Errore: %1 ha restituito %2\n"
MFM_COMP_ERR	"Errore nella compilazione di %1\n"

$domain LCC.MERGE.UNIX.WINSETUP
$ Messages for the "winsetup" script.  This scripts runs when the
$ "Win_Setup" icon is used, or the user types "winsetup".
$ There are a maximum of 24 lines of message that can be printed out,
$ altough all don't have to be used.
$ Also, a single line message can be put into "SPCL_MSG", which is printed
$ out before the batch scripts are run.
$ All these messages are printed on the DOS screen, so these messages
$ must be in the appropriate DOS code page.
LINE01 "ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿"
LINE02 "³     Usare questa sessione DOS per installare e riconfigurare Windows 3.1    ³"
LINE03 "ÃÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ´"
LINE04 "³                                                                             ³"
LINE05 "³ - Per installare una copia personale di Windows, inserire il Disco 1        ³"
LINE06 "³   di Windows e digitare quanto segue, se si installa dall'unit… A:          ³"
LINE07 "³                                                                             ³"
LINE08 "³                a:\\setup                                                     ³"
LINE09 "³                                                                             ³"
LINE10 "³   (o b:\\setup se si installa dall'unit… B:)                                 ³"
LINE11 "³                                                                             ³"
LINE12 "³   Consultate il manuale per una descrizione passo-passo della procedura di  ³"
LINE13 "³   installazione. Il manuale descrive anche varie opzioni di installazione.  ³"
LINE14 "³                                                                             ³"
LINE15 "³ - Per riconfigurare Windows, prima cambiare l'unit… e la directory          ³"
LINE16 "³   in cui Š stato installato Windows, poi digitare:                          ³"
LINE17 "³                                                                             ³"
LINE18 "³                setup                                                        ³"
LINE19 "³                                                                             ³"
LINE20 "ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ"
$quote "
$domain LCC.PCI.UNIX.LSET_ESTR
$ The following messages are from lset_estr.c
LSET_ESTR0 "non è un errore"
LSET_ESTR1 "errore di sistema"
LSET_ESTR2 "errore di parametro"
LSET_ESTR3 "tabella con lock/senza lock"
LSET_ESTR4 "impossibile mettere/togliere lock a tabella"
LSET_ESTR5 "impossibile acquisire identificatore IPC"
LSET_ESTR6 "impossibile creare/accedere a lock set"
LSET_ESTR7 "impossibile collegare segmento di memoria condivisa"
LSET_ESTR8 "manca spazio di memoria/tabella"
LSET_ESTR9 "lo slot specificato è in uso"
LSET_ESTR10 "lo slot specificato non è in uso"
LSET_ESTR11 "il lockset esiste già"
LSET_ESTR12 "Permesso negato"
LSET_ESTR13 "errore LSERR sconosciuto (%d)"

$ @(#)messages	4.5 9/6/94 14:20:37

$quote "
$domain LCC.PCI.UNIX.RLOCKSHM

$ The following messages are from rlockshm.c
RLOCKSHM6 "Config è valido solo con memoria condivisa del nuovo tipo.\n"
RLOCKSHM7 "Impossibile %1 memoria condivisa comune, "
RLOCKSHM8 "(errore di sistema: %1).\n"
RLOCKSHM9 "%1.\n"
RLOCKSHM10 "\nInformazioni della configurazione di default:\n\n"
RLOCKSHM11 "\tindirizzo di collegamento segmento\t\t(selezionato da programma)\n"
RLOCKSHM12 "\tindirizzo di collegamento segmento\t\t%1\n"
RLOCKSHM14 "\tchiave memoria condivisa\t\t%1\n"
RLOCKSHM15 "\tchiave lockset\t\t\t%1\n"
RLOCKSHM17 "\tvoci di tabella dei file aperti\t\t%1\n"
RLOCKSHM18 "\tvoci di tabella delle intestazioni di file\t%1\n"
RLOCKSHM19 "\tvoci di tabella dei file hash\t%1\n"
RLOCKSHM20 "\tvoci di tabella dei lock di record\t%1\n"
RLOCKSHM22 "\tsingoli lock di record\t\t%1\n"
RLOCKSHM24 "Questi dati possono essere modificati con questi"
RLOCKSHM25 " nomi di parametri configurabili:\n"
RLOCKSHM27 "\t%1 - %2\n"
RLOCKSHM28 "\nInformazioni varie sulla memoria esistente:\n\n"
RLOCKSHM29 "\tvoci di tabella dei file aperti\t\t%1 @ %2 byte\n"
RLOCKSHM30 "\tvoci di tabella delle intestazioni di file\t%1 @ %2 byte\n"
RLOCKSHM31 "\tvoci di tabella dei file hash\t%1 @ %2 byte\n"
RLOCKSHM32 "\tvoci di tabella dei lock di record\t%1 @ %2 byte\n"
RLOCKSHM34 "\tbase di segmento aperto\t\t%1\n"
RLOCKSHM35 "\tbase di segmento di file\t\t%1\n"
RLOCKSHM36 "\tbase di segmento di lock\t\t%1\n"
RLOCKSHM38 "\ttabella dei file aperti\t\t\t%1\n"
RLOCKSHM39 "\ttabella delle intestazioni di file\t\t%1\n"
RLOCKSHM40 "\ttabella dei file hash\t\t%1\n"
RLOCKSHM41 "\ttabella dei lock di record\t\t%1\n"
RLOCKSHM43 "\tlista libera tabella aperti\t\t%1\n"
RLOCKSHM44 "\tlista libera tabella file\t\t%1\n"
RLOCKSHM45 "\tlista libera tabella lock\t\t%1\n"
RLOCKSHM46 "\tvoci di tabella dei file aperti\t\t%1 @ %2 byte\n"
RLOCKSHM47 "\tvoci di tabella delle intestazioni di file\t%1 @ %2 byte\n"
RLOCKSHM48 "\tvoci di tabella dei file hash\t%1 @ %2 byte\n"
RLOCKSHM49 "\tvoci di tabella dei lock di record\t%1 @ %2 byte\n"
RLOCKSHM50 "\tdimensione totale segmento\t\t%1 byte\n"
RLOCKSHM52 "\tsingoli lock di record\t\t%1\n"
RLOCKSHM54 "\tbase di collegamento\t\t\t%1"
RLOCKSHM55 " (selezionata da programma)"
RLOCKSHM57 "\ttabella dei file aperti\t\t\t%1\n"
RLOCKSHM58 "\ttabella delle intestazioni di file\t\t%1\n"
RLOCKSHM59 "\ttabella dei file hash\t\t%1\n"
RLOCKSHM60 "\ttabella dei lock di record\t\t%1\n"
RLOCKSHM62 "\tindice lista libera tabella aperti\t%1\n"
RLOCKSHM63 "\tindice lista libera tabella file\t%1\n"
RLOCKSHM64 "\tindice lista libera tabella lock\t%1\n"
RLOCKSHM65 "\tfattore di limitazione allineamento\t%1\n"
RLOCKSHM66 "\nVoci di tabella delle intestazioni di file:\n"
RLOCKSHM100 "voce"
RLOCKSHM101 "lista-aperti"
RLOCKSHM102 "lista-lock"
RLOCKSHM103 "hash-link"
RLOCKSHM104 "ID-unica"
RLOCKSHM105 "indice-aperti"
RLOCKSHM106 "indice-lock"
RLOCKSHM70 "\nVoci di tabella delle intestazioni dei file hash:\n"
RLOCKSHM200 "voce"
RLOCKSHM201 "int-file"
RLOCKSHM75 "\nVoci di tabella dei lock di record:\n"
RLOCKSHM111 "lock-succ"
RLOCKSHM112 "lock-basso"
RLOCKSHM113 "lock-alto"
RLOCKSHM114 "ID-sess"
RLOCKSHM115 "PID-dos"
RLOCKSHM116 "ind-succ"
RLOCKSHM80 "\nVoci di tabella dei file aperti:\n"
RLOCKSHM120 "aperto-succ"
RLOCKSHM121 "intestazione"
RLOCKSHM122 "acc"
RLOCKSHM123 "nega"
RLOCKSHM130 "ind-intf"
RLOCKSHM131 "desc-file"
RLOCKSHM_OLDST	"AVVISO: Memoria di tipo vecchio in uso -- è obsoleta\n"
RLOCKSHM_USAGE	"\nuso: %1 [-cdhmrAFHLOV] [nome=dati] ...\n"
RLOCKSHM_DETAIL "\
\  -c  Crea il segmento di memoria condivisa.\n\
\  -d  Visualizza (solo) la configurazione di default.\n\
\  -h  Visualizza (solo) queste informazioni.\n\
\  -m  Visualizza informazioni varie sul segmento esistente.\n\
\  -r  Rimuovi il segmento di memoria condivisa.\n\
\  -A  Visualizza tutte le voci (anche quelle inutilizzate).\n\
\  -F  Visualizza la tabella intestazioni file.\n\
\  -H  Visualizza la tabella intestazioni file hashed.\n\
\  -L  Visualizza la tabella di lock dei record.\n\
\  -O  Visualizza la tabella dei file aperti.\n\
\  -V  Stampa (solo) versione/copyright.\n\
\  nome=dati - Zero o più impostazioni di configurazione.\n\n"

$ The following messages are from set_cfg.c
$quote "
SET_CFG1 "Stringa di configurazione non valida: '%1'\n"
SET_CFG2 "Non è stato fornito un nome per '%1'.\n"
SET_CFG3 "Il nome in '%1' è troppo lungo.\n"
SET_CFG4 "Non sono stati forniti dati per '%1'.\n"
SET_CFG5 "Nome sconosciuto: '%1'\n"
SET_CFG6 "Il dato per '%1' deve essere non negativo.\n"
SET_CFG7 "Il dato per '%1' non è valido.\n"
SET_CFG8 "Il dato per '%1' deve essere minore di %2.\n"

$ The following are handcrafted from set_cfg.c
SET_CFG100 "base"
SET_CFG101 "indirizzo di collegamento segmento (0 = selezionato da programma)"
SET_CFG102 "chiavi"
SET_CFG103 "impostare la memoria condivisa e le chiavi lockset' LSW"
SET_CFG104 "tabaperti"
SET_CFG105 "numero massimo di voci di tabella dei file aperti"
SET_CFG106 "tabfile"
SET_CFG107 "numero massimo di voci di tabella delle intestazioni dei file"
SET_CFG108 "tabhash"
SET_CFG109 "numero massimo di voci di tabella dei file hash"
SET_CFG110 "tablock"
SET_CFG111 "massimo numero di voci di tabella dei lock di record"
SET_CFG112 "lockrec"
SET_CFG113 "numero massimo di lock di record singoli"
$ SCCSID(@(#)messages	7.11	LCC)	/* Modified: 10:33:25 10/20/93 */

$quote "
$domain LCC.PCI.UNIX

$ In general, the names below start with some indication of the file in
$ which the string is actually used.  Usually, this is the base name of
$ the file, although it may be some "shorter" version of the name.

LICM_VIOLATION	"Violazione di licenza, %1 (terminato) vs. %2\n"
LICM_TERMINATE	"Violazione di licenza -- server terminato\n"
LICM_BAD_KEY	"Licenza non valida (chiave sbagliata)\n"
LICM_ALTERED	"Licenza non valida (testo alterato)\n"
LICM_NO_ID	"Licenza non valida (manca ID di licenza)\n"
LICM_NO_MEMORY	"Licenza inutilizzabile (memoria insufficiente)\n"
LICM_EXPIRED	"Licenza inutilizzabile (scaduta)\n"
LICM_NO_SERIAL	"Manca la specifica del numero di serie del client\n"
LICM_BAD_SERIAL	"Specifica di numero di serie del client non valida\n"

LICU_DUPLICATE	"Numero di serie del client duplicato, connessione non consentita."
LICU_INVALID	"Numero di serie del client non valido, connessione non consentita."
LICU_RESOURCE	"La risorsa host non è disponibile, consultare l'amministratore di sistema."

$ %1 is the maximum number of clients
LICU_LIMIT	"Questo server ha raggiunto il suo limite di client con licenza (%1), connessione impossibile."

$ %1 is the log file name, %2 is the error string, %3 and %4 (where used) are
$ the user and group IDs, respectively
LOG_OPEN	"Impossibile aprire file di log '%1', %2\n"
LOG_CHMODE	"Impossibile cambiare modo di '%1', %2\n"
LOG_CHOWN	"Impossibile cambiare il proprietario di '%1' in %3/%4, %2\n"
LOG_REOPEN	"Impossibile riaprire '%1' dopo il troncamento, %2\n"

$ %1 is a host file descriptor
LOG_OPEND	"Impossibile riaprire il log dal descrittore %1\n"

$ %1 is the program name, %2 is the process id
LOG_SERIOUS	"Grave errore in %1 (PID %2), tentativo di prosecuzione\n"
LOG_FATAL	"Errore fatale in %1 (PID %2), impossibile proseguire\n"

$ %1 is a number of bytes
MEM_NONE	"Impossibile allocare %1 byte di memoria.\n"
MEM_RESIZE	"Impossibile ridimensionare la memoria a %1 byte\n"

$ %1 is the host name
NETAD_NOHOST	"Impossibile trovare dati sull'host '%1'\n"

$ %1 is the error string
NETIF_DEVACC	"Impossibile accedere al dispositivo di rete, %1\n"
NETIF_CONFIG	"Impossibile determinare la configurazione della rete, %1\n"

NETIO_DESC_ARR	"Impossibile allocare l'array descrittore della rete\n"
NETIO_RETRY	"Impossibile ricevere un pacchetto dalla rete\n"
NETIO_CORRUPT	"La tabella interna della rete è alterata\n"
NETIO_MUXERR	"Errore di multiplexaggio della rete\n"

$ %1 is the size of a network packet
NETIO_PACKET	"La dimensione massima del pacchetto della rete (%1) è troppo piccola\n"

$ %1 is a network address
NETIO_IFERR	"Errore irrecuperabile sull'interfaccia %1\n"

DOSSVR_NO_CSVR	"Impossibile trovare l'indirizzo del server di connessione\n"
DOSSVR_SETIF	"Impossibile aprire l'interfaccia di rete locale\n"
DOSSVR_CURDIR	"Impossibile determinare la directory di lavoro attuale\n"

$ %1 is the error string
DOSSVR_R_CPIPE	"Impossibile leggere il pipe di configurazione, %1\n"
DOSSVR_G_NETA	"Impossibile acquisire gli attributi della rete, %1\n"
DOSSVR_S_NETA	"Impossibile impostare gli attributi della rete, %1\n"
DOSSVR_G_TERMA	"Impossibile acquisire gli attributi della linea terminale, %1\n"
DOSSVR_S_TERMA	"Impossibile impostare gli attributi della linea terminale, %1\n"
DOSSVR_C_CPIPE	"Impossibile creare il pipe di configurazione, %1\n"
DOSSVR_NOFORK	"Impossibile creare un nuovo processo, %1\n"

$ %1 is an RLOCK package error, %1 is a system error
DOSSVR_RLINIT	"Impossibile inizializzare i dati di lock di record, %1, %2\n"

$ %1 is a program name, %2 is an error string (if used)
DOSSVR_NOEXEC	"Impossibile avviare '%1', %2\n"
DOSSVR_NOSHELL	"Impossibile avviare la shell utente '%1'\n"
DOSSVR_ACC_SVR	"Impossibile accedere al server DOS '%1'\n"
DOSSVR_RUN_SVR	"Impossibile eseguire il server DOS '%1', %2\n"

$ %1 is an luid, %2 is the error string
DOSSVR_LUID	"Impossibile impostare Iuid a %1, %2\n"

$ %1 is the written count, %2 is the expected count, %3 is the error string
DOSSVR_W_CPIPE	"Impossibile scrivere il pipe di configurazione (%1 byte di %2), %3\n"

CONSVR_NOMEM	"Impossibile allocare memoria per la stringa della funzione\n"
CONSVR_NO_NET	"Impossibile aprire le interfacce di rete\n"

$ %1 is the luid that started the consvr process
CONSVR_LUID	"La Iuid è già impostata a %1\n"

$ %1 is file or program name, %2 is error string (where used)
CONSVR_RUN_SVR	"Impossibile eseguire il server DOS '%1', %2\n"
CONSVR_NO_FF	"Impossibile aprire il file delle funzioni, '%1'\n"
CONSVR_ERR_FF	"Errore nel file delle funzioni '%1'\n"

$ %1, %2, %3 and %4 are the major, minor, sub-minor and special version ID
$ values, respectively
CONSVR_BANNER		"PC-Interface per DOS, Versione %1.%2.%3 %4\n"
CONSVR_BANNER_AADU	"DOS Server per AIX, Versione %1.%2\n"

$ %1 is error string
IPC_NO_MSGQ	"Impossibile creare una coda di messaggi, %1\n"
IPC_NO_SEM	"Impossibile creare un semaforo, %1\n"

MAPSVR_NO_NET	"Impossibile aprire le interfacce di rete\n"

DOSOUT_SEGMENT	"Impossibile accedere al segmento di memoria condivisa RD\n"
DOSOUT_REXMIT	"Troppe ritrasmissioni\n"

$ %1 is an error string
DOSOUT_NO_SHM	"Memoria condivisa esaurita, %1\n"
DOSOUT_S_NETA	"Impossibile impostare gli attributi della rete, %1\n"
DOSOUT_PIPE_ACK	"Errore di I/O del pipe (ACK wait), %1\n"
DOSOUT_PIPE_CNT	"Errore di I/O del pipe (ACK control), %1\n"
DOSOUT_ERR6	"PTY Errore lettura 6: %1\n"
DOSOUT_ERR7	"PTY Errore lettura 7: %1\n"
DOSOUT_ERR8	"PTY Errore lettura 8: %1\n"
DOSOUT_ERR13	"PTY Errore lettura 13: %1\n"
DOSOUT_ERR14	"PTY Errore lettura 14: %1\n"
DOSOUT_ERR19	"PTY Errore lettura 19: %1\n"
DOSOUT_ERR9	"TTY Errore scrittura 9: %1\n"
DOSOUT_ERR10	"TTY Errore scrittura 10: %1\n"
DOSOUT_ERR15	"TTY Errore scrittura 15: %1\n"
DOSOUT_ERR16	"TTY Errore scrittura 16: %1\n"
DOSOUT_ERR17	"TTY Errore scrittura 17: %1\n"

$ %1 is an error string
SEMFUNC_NOSEM	"Impossibile creare un semaforo RD, %1\n"
SEMFUNC_NOSHM	"Impossibile creare un segmento di memoria condivisa RD, %1\n"

$ %1 is the number of objects in the caches, %2 is the size of each object
VFILE_NOMEM	"Impossibile allocare memoria per %1 oggetti, di %2 byte ciascuno\n"

$ %1 is the lcs error number
NLSTAB_HOST	"Impossibile accedere alla tabella LCS dell'host, errore %1\n"
NLSTAB_CLIENT	"Impossibile accedere alla tabella LCS del client, errore %1\n"
NLSTAB_SET	"Impossibile impostare le tabelle LCS, errore %1\n"

DEBUG_CHILD	"inferiore"
DEBUG_OFF	"no"
DEBUG_ON	"sì"

$ %1 is the program name
DEBUG_USAGE	"Uso: %1 <PID> [[op]canali] [inferiore] [on] [off]\n"

$ %1 is the program name, %2 is the faulty command line argument
DEBUG_ARG	"%1: Argomento non valido: '%2'\n"

$ %1 is the program name, %2 is the channel file, %3 is an error string
DEBUG_NOFILE	"%1: Impossibile creare il file di canale '%2', %3\n"

$ %1 is the program name, %2 is a PID, %3 is an error string
DEBUG_INVAL	"%1: Il processo %2 non può essere segnalato, %3\n"
DEBUG_NO_SIG	"%1: Impossibile segnalare il processo %2, %3\n"

$ %1 is the program name, %2 is a channel number specified in the argument list
DEBUG_BADBIT	"%1: Il canale %2 è fuori dall'intervallo\n"
$ SCCSID("@(#)messages	7.2	LCC")	/* Modified: 1/19/93 20:08:07 */
$domain LCC.PCI.DOS.CONVERT
$quote "
$ NOTE: '\n' indicates that a new line will be printed
$ The following messages are from convert.c
CONVERT1 "Specificato SIA maiuscolo SIA minuscolo\n"
CONVERT1A "Specificate opzioni incompatibili\n"
CONVERT2 "Specificate traduzioni in conflitto\n"

$ %1 is the file name on which a read error occured
CONVERT3 "Si è verificato un errore durante la lettura di %1\n"

$ %1 is the fiel name which cannot be opened
CONVERT4 "Impossibile aprire %1\n"

$ input file %1 and output file %2 are identical
CONVERT5 "I file %1 e %2 sono identici\n"

$ an error was encountered writing file %1
CONVERT6 "Si è verificato un errore durante la scrittura di %1\n"

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
CONVERT_S3 "Grazie tradotte con più byte:\t%1\n"
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
CONVERT_M5_D "\                /c c    Usare c come user_char di errore di traduzione.\n\
\                /m      Permettere la traduzione di caratteri multipli.\n\
\                /a      Interrompere nel caso di un errore di traduzione.\n\
\                /s      Usare la traduzione migliore a carattere singolo.\n\
\                /z      Trattare correttamente CTRL-Z.\n\
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
\                -x      Diretto.  Non tradurre.\n\n\
\                -i tbl  Traduci l'input usando la tabella tbl.\n\
\                -o tbl  Traduci l'output usando la tabella tbl.\n\
\n"
CONVERT_M5_U "\                -c c    Usare c come user_char di errore di traduzione.\n\
\                -m      Permettere la traduzione di caratteri multipli.\n\
\                -a      Interrompere in caso di errore di traduzione.\n\
\                -s      Usare la traduzione migliore a carattere singolo.\n\
\                -z      Trattare correttamente CTRL-Z.\n\
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
