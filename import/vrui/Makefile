########################  import/vrui/Makefile  ################################
#
SHELL=/bin/bash

.PHONY: download all

all: download
	rm $G/import/vrui/src/Share/Vrui.cfg
	cp $G/import/vrui/Vrui.cfg $G/import/vrui/src/Share
	rm $G/import/vrui/src/Share/VRDevices.cfg
	cp $G/import/vrui/VRDevices.cfg $G/import/vrui/src/Share
	rm $G/import/vrui/src/GL/GLWindow.cpp
	cp $G/import/vrui/GLWindow.cpp $G/import/vrui/src/GL/
	cd $G/import/vrui/src; make
	cd $G/import/vrui/src; make install

# Only download the whole thing if it's older than half a day.
download:
	if [ ! -d src ] || [ $$(( $(shell date +'%s') - \
	  $(shell stat -c '%Y' src) )) -gt 43200 ]; then \
	    rm -rf src ; \
	    wget -O src.tar.gz http://www.idav.ucdavis.edu/~okreylos/ResDev/Vrui/Vrui-3.1-002.tar.gz ; \
	    mkdir src && tar xzvf src.tar.gz -C src --strip-components 1 ; \
	    rm src.tar.gz ; \
	    sed -i '/INSTALLDIR := $$(HOME)/c\INSTALLDIR := $(G)\/install_linux' $G/import/vrui/src/makefile ; \
	fi

#
####################  end  import/vrui/Makefile  ###############################
