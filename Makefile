#############################################################################
#	
#	makefile for building:
#
#		libmcchid.so:        Library for USB series
#		mysql-usb1608G:       Program to test USB-1608G module
#
#               Copyright (C)  2009-2013
#               Written by:  Warren J. Jasper
#                            North Carolina State Univerisity
#
#
# This program, libmcchid.so, is free software; you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version, provided that this
# copyright notice is preserved on all copies.
#
# ANY RIGHTS GRANTED HEREUNDER ARE GRANTED WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND FURTHER,
# THERE SHALL BE NO WARRANTY AS TO CONFORMITY WITH ANY USER MANUALS OR
# OTHER LITERATURE PROVIDED WITH SOFTWARE OR THAM MY BE ISSUED FROM TIME
# TO TIME. IT IS PROVIDED SOLELY "AS IS".
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
###########################################################################

#  Current Version of the driver
VERSION=1.54

SRCS = pmd.c usb-1608G.c 
HEADERS = pmd.h usb-1608G.h
OBJS = $(SRCS:.c=.o)   # same list as SRCS with extension changed
CC=gcc
CFLAGS= -g -Wall -fPIC -O 
TARGETS=libmcchid.so libmcchid.a mysql-usb1608G 
ID=MCCLIBHID
DIST_NAME=$(ID).$(VERSION).tgz
DIST_FILES={README,Makefile,usb-1608G.h,usb-1608G.c,usb-1608G.rbf,mysql-usb1608G.c}

###### RULES
all: $(TARGETS)

%.d: %.c
	set -e; $(CC) -I. -M $(CPPFLAGS) $< \
	| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	[ -s $@ ] || rm -f $@
ifneq ($(MAKECMDGOALS),clean)
include $(SRCS:.c=.d)
endif

libmcchid.so: $(OBJS)
#	$(CC) -O -shared -Wall $(OBJS) -o $@
	$(CC) -shared -Wl,-soname,$@ -o $@ $(OBJS) -lc -lm

libmcchid.a: $(OBJS)
	ar -r libmcchid.a $(OBJS)
	ranlib libmcchid.a

#http://www.daniweb.com/software-development/c/threads/180705/how-to-include-mysql.h-in-makefile
MYSQLCFLAGS= -I/usr/include/mysql -DBIG_JOINS=1 -fno-strict-aliasing -g
MYSQLLIBS= -L/usr/lib/arm-linux-gnueabihf -lmysqlclient -lpthread -lz -lm -lrt -ldl


mysql-usb1608G:	mysql-usb1608G.c usb-1608G.o libmcchid.a
	$(CC) -g -Wall -I. -o $@ $@.c -L. -lmcchid  -lm -L/usr/local/lib -lhid -lusb $(MYSQLCFLAGS) $(MYSQLLIBS)

clean:
	rm -rf *.d *.o *~ *.a *.so $(TARGETS)

dist:	
	make clean
	cd ..; tar -zcvf $(DIST_NAME) libhid/$(DIST_FILES);

install:
	-install -d /usr/local/lib
	-install -c --mode=0755 ./libmcchid.a libmcchid.so /usr/local/lib
	-/bin/ln -s /usr/local/lib/libmcchid.so /usr/lib/libmcchid.so
	-/bin/ln -s /usr/local/lib/libmcchid.a /usr/lib/libmcchid.a
	-install -d /usr/local/include/libhid
	-install -c --mode=0644 ${HEADERS} /usr/local/include/libhid/

uninstall:
	-rm -f /usr/local/lib/libmcchid*
	-rm -f /usr/lib/libmcchid*
	-rm -rf /usr/local/include/libhid
