.ul
Instalación y configuración del servicio de información de red (NIS)
.lr
F1=Ayuda
.top
`echo  "\n \nHa escogido la siguiente configuración:"`
`echo  "\n \n  $host es el NIS $host_type"` 
`echo  "\n \n  dominio NIS: $def_dom"`
`[ "$slavep" = "F" ] && echo  "\n \n  Los servidores NIS del dominio $def_dom son: $SERV1"`
`[ "$slavep" = "F" ] && echo  "                                       $SERV2"`
`[ "$slavep" = "F" ] && echo  "                                       $SERV3"`
`[ "$slavep" = "T" ] && echo  "\n \n  El servidor maestro NIS del dominio $def_dom es: $master"`
`echo   "\n \nTeclee No para volver a configurar o para cancelar la configuración de\nNIS."`
.button
Aplicar
Redefinir
.bottom
Pulse <Tab> para mover el cursor entre los campos. Al terminar, mueva el
cursor a "Aplicar" y pulse <Intro> para continuar.
.form
2 2//No::No//Yes::Sí//¿Aceptar configuración NIS?://ACCEPT//
//Teclee la flecha derecha para escoger Si y aceptar la configuración//
.help
Si esta configuración de NIS no es aceptable, introduzca "No" y
dispondrá de la oportunidad de volver a configurar el NIS o de cancelar
la configuración en el menú subsiguiente.  Independientemente de si
cancela la configuración de NIS, NIS se instalará igualmente.

Nota: Este menú sólo muestra los tres primeros servidores NIS 
      configurados para este dominio.
.helpinst
Supr=Cancelar F1=Ayuda ESC=Salir de Ayuda 1=Av Pág 2=Re Pág
