#!/bin/bash

#SE COMPRUEBA SI EL NOMBRE DEL ZIP SIGUE EL FORMATO
RES=`echo "$1" | grep -E '^ssoo_p3(_100[[:digit:]]{6}){1,2}.zip$'`
if [ "$RES" = "" ] ;then
	echo "ZIP name/format ERROR"
	exit 1
fi
echo "ZIP name/format OK"

#SE COMPRUEBA SI EL FICHERO EXISTE
if [ ! -f "$1" ] ;then
	echo "ZIP does not exist"
	exit 1
fi

#SE DESCOMPRIME EL ZIP EN UNA CARPETA SEPARADA PARA LAS PRUEBAS
unzip $1 -d unzip
cd unzip/ssoo_p3_concurrencia

#SE COMPRUEBA QUE EL ZIP CONTIENE LOS FICHEROS y Makefile
if [ -f "factory.c" ]
then
	echo -e "factory.c OK"
else
	echo -e "factory.c ERROR"
	exit 1
fi

if [ -f "./include/factory.h" ]
then
	echo -e "factory.c OK"
else
	echo -e "factory.c ERROR"
	exit 1
fi

if [ -f "./include/db_factory.h" ]
then
	echo -e "db_factory.c OK"
else
	echo -e "db_factory.c ERROR"
	exit 1
fi
if [ -f "./lib/libdb_factory.a" ]
then
	echo -e "libdb_factory.a OK"
else
	echo -e "libdb_factory.a ERROR"
	exit 1
fi



if [ -f "Makefile" ]
then
	echo -e "Makefile OK"
else
	echo -e "Makefile ERROR"
	exit 1
fi


#SE COMPRUEBA SI EL PROGRAMA COMPILA
make &> compilation
if [ -f factory.exe ]
then
	echo -e "Compilation OK"
else
	echo -e "Compilation ERROR"
	exit 1
fi




#SE ELIMINA LA CARPETA DE PRUEBAS
cd ..
rm -rf unzip
