.ul
Installazione e configurazione di Network Information Service (NIS)
.lr
F1=Guida
.top
`echo  "\n \n» stata scelta la seguente configurazione NIS:"`
`echo  "\n \n  $host Ë un $host_type NIS"` 
`echo  "\n \n  dominio NIS: $def_dom"`
`[ "$slavep" = "F" ] && echo  "\n \n  server NIS per il dominio $def_dom: $SERV1"`
`[ "$slavep" = "F" ] && echo  "                                       $SERV2"`
`[ "$slavep" = "F" ] && echo  "                                       $SERV3"`
`[ "$slavep" = "T" ] && echo  "\n \n  server NIS master per il dominio $def_dom: $master"`
`echo   "\n \nPer rifare o annullare la configurazione NIS immettere No."`
.button
Applica
Reimposta
.bottom
Premere <Tab> per portare il cursore da un campo all'altro. Alla fine,
portare il cursore su "Applica" e premere <Invio> per continuare.
.form
2 2//No::No//Yes::SÏ//Accettare configurazione NIS?://ACCEPT//
//Freccia sinistra/destra per scegliere SÏ (accettare configurazione)//
.help
Se questa configurazione NIS non Ë accettabile, immettere "No"; in un
menu successivo, vi verr‡ data la possibilit‡ di ripetere o di annullare
la configurazione di NIS. Sia che la configurazione venga effettuata,
sia che venga annullata, NIS verr‡ installato.

Nota: Questo menu mostra solo i primi tre server NIS configurati per
      questo dominio.
.helpinst
Canc=Annulla  F1=Guida  ESC=Fine guida  1=Avanti  2=Indietro
