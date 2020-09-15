# Shared make definitions
# Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
# listed in the AUTHORS file in the MIT Proto distribution's top directory.
# 
# This file is part of MIT Proto, and is distributed under the terms of
# the GNU General Public License, with a linking exception, as described
# in the file LICENSE in the MIT Proto distribution's top directory.

CC = gcc 
CXX = g++
AR	= ar
ARFLAGS	=

.PHONY : clean force_look

%.o: %.c %.h
	$(CC) -c ${FLAGS} $*.c

%.o: %.c
	$(CC) -c ${FLAGS} $<

%.o: %.cpp %.h
	$(CXX) -c ${FLAGS} $<

%.o: %.cpp
	$(CXX) -c ${FLAGS} $<
