#!/usr/bin/bash

# $Id$ 

# build libjpeg
if [ ! -d libjpeg ]
then
	tar -zxvf ./downloads/jpegsrc.v6b.tar.gz
	mv ./jpeg-6b ./libjpeg
	cd ./libjpeg
	./configure
	make
	cd ../
	patch -p0 < ./patches/jmorecfg.h.patch ;
else
	echo "+++ libjpeg directory already exists +++" ;
fi

# build zlib
if [ ! -d zlib ] 
then
	tar -zxvf ./downloads/zlib-1.2.1.tar.gz
	mv ./zlib-1.2.1 ./zlib
	cd ./zlib
	./configure
	make
	cd ../
	patch -p0 < ./patches/zconf.h.patch
else
	echo "+++ zlib directory already exists +++" ;
fi
