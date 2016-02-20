########################  import/3dvisualizer/Makefile  ########################
#
SHELL=/bin/bash

.PHONY: download all

all: download
	cd $G/import/3dvisualizer/src; make
	cd $G/import/3dvisualizer/src; make install

download:
	if [ ! -d src ] || [ $$(( $(shell date +'%s') - \
	  $(shell stat -c '%Y' src) )) -gt 43200 ]; then \
	    rm -rf src ; \
	    wget -O src.tar.gz https://github.com/KeckCAVES/3DVisualizer/archive/v1.14.tar.gz ; \
	    mkdir src && tar xzvf src.tar.gz -C src --strip-components 1 ; \
	    rm src.tar.gz ; \
	    sed -i '/VRUI_MAKEDIR := $$(HOME)/c\VRUI_MAKEDIR = $(G)\/import\/vrui\/src\/BuildRoot' $G/import/3dvisualizer/src/makefile ; \
	    sed -i '/INSTALLDIR := $$(shell pwd)/c\INSTALLDIR = $(G)\/install_linux' $G/import/3dvisualizer/src/makefile ; \
	fi

#
####################  end  import/3dvisualizer/Makefile  #######################
