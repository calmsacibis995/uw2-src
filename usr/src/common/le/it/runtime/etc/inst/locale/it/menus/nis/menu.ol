.ul
Installazione e configurazione di Network Information Service (NIS)
.lr
F1=Guida
.top
`echo  "La seguente configurazione NIS esiste gi� su questa macchina."`
`echo "\n \n \n"`
`[ "$TYPE" = "1" ] && echo "    Questa macchina � un server NIS master.\n \n"`
`[ "$TYPE" = "2" ] && echo "    Questa macchina � un server NIS slave.\n \n"`
`[ "$TYPE" = "3" ] && echo "    Questa macchina � un client NIS.\n \n"`
`echo "    dominio NIS: $def_dom"` 
`echo "\n \n    server NIS per il dominio $def_dom: \c" && cat /tmp/nis.overlay | xargs echo`
.button
Applica
Reimposta
.bottom
Premere <Tab> per portare il cursore da un campo all'altro. Alla fine,
portare il cursore su "Applica" e premere <Invio> per continuare.
.form
2 2//Yes::S�//No::No//Usare configurazione NIS attuale?://USE_CURRENT//
//Freccia sinistra/destra per scegliere S� (configurazione attuale)//
.help
Questa macchina � gi� stata configurata con NIS. L'installazione pu�
procedere usando la configurazione NIS attuale (immettere S�) oppure pu�
essere creata una nuova configurazione NIS con lo script di
installazione NIS (immettere No).

Nota: Questo menu mostra solo i primi tre server NIS configurati per
      questo dominio.
.helpinst
Canc=Annulla  F1=Guida  ESC=Fine guida  1=Avanti  2=Indietro
