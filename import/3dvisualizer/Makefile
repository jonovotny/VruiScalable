########################  import/3dvisualizer/Makefile  ########################
#
SHELL=/bin/bash

.PHONY: all

all:
	sed -i '/VRUI_MAKEDIR := $$(HOME)/c\VRUI_MAKEDIR = $(G)\/import\/vrui\/src\/BuildRoot' $G/import/3dvisualizer/src/makefile ; \
	sed -i '/INSTALLDIR := $$(shell pwd)/c\INSTALLDIR = $(G)\/install_linux' $G/import/3dvisualizer/src/makefile ; \
	cd $G/import/3dvisualizer/src; make
	cd $G/import/3dvisualizer/src; make install
#
####################  end  import/3dvisualizer/Makefile  #######################
