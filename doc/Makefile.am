##
## doc/Makefile.am
##
## Copyright (C) 2000 Jeffrey Fulmer <jdfulmer@armstrong.com>
## This file is part of Siege
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
##

AUTOMAKE_OPTIONS  =    foreign no-dependencies 

man_MANS          =    scout.1                

SCOUTRC           =    $(HOME)/.scoutrc

EXTRA_DIST        =    $(man_MANS) scoutrc.in

install-exec-hook:
	@if test -f $(SCOUTRC); then \
	  if cmp -s $(srcdir)/scoutrc $(SCOUTRC); then echo ""; \
	  else \
          echo ' $(INSTALL_DATA) $(srcdir)/scoutrc $(SCOUTRC).new'; \
	    $(INSTALL_DATA) $(srcdir)/scoutrc $(SCOUTRC).new; \
	    echo "#####################################################"; \
	    echo "WARNING: File $(SCOUTRC) already exists."; \
	    echo "         A new resource file has been installed as"; \
	    echo "         $(SCOUTRC).new. You may want to"; \
	    echo "         consider using the newer version in order to"; \
	    echo "         take advantage of any new features."; \
	    echo "#####################################################"; \
	  fi; \
	else \
	  $(INSTALL_DATA) $(srcdir)/scoutrc $(SCOUTRC); \
	fi 

