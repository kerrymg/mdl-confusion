#   'Confusion', a MDL intepreter
#   Copyright 2009 Matthew T. Russotto
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, version 3 of 29 June 2007.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

CDEBUGFLAGS = -g -DGC_DEBUG
COPTFLAGS = -O2
CWARNFLAGS =  -Wall -Wno-switch
LIBS = -lgc -lgccpp
CFLAGS = -Isrc $(CDEBUGFLAGS) $(COPTFLAGS) $(CWARNFLAGS)
CXXFLAGS = -std=c++11 -Isrc $(CDEBUGFLAGS) $(COPTFLAGS) $(CWARNFLAGS)

PERL = perl

PROGRAMS = mdli

TMPSRCS = src/mdl_builtins.cpp src/mdl_builtin_types.cpp src/mdl_builtin_types.h src/mdl_builtins.h src/license.c

CXXSRCS = src/macros.cpp \
src/mdli.cpp \
src/mdl_builtins.cpp \
src/mdl_builtin_types.cpp \
src/mdl_read.cpp \
src/mdl_output.cpp \
src/mdl_binary_io.cpp \
src/mdl_decl.cpp \
src/mdl_assoc.cpp

CSRCS = src/mdl_strbuf.c src/license.c

OBJS = $(CXXSRCS:.cpp=.o) $(CSRCS:.c=.o)

mdli: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

src/license.c: LICENSE
	awk 'BEGIN { print "const char license [] = " } /END OF TERMS AND CONDITIONS/ { nextfile } { gsub("\"", "\\\"", $$0); print "\"" $$0 "\\n\""  } END { print ";" }'  < LICENSE > src/license.c

src/mdl_builtins.o: src/mdl_builtins.h

src/mdl_builtins.cpp: src/macros.cpp
	$(PERL) scripts/find_builtins.pl $(@:.cpp=.h) < $< > $@	

src/mdl_builtin_types.o: src/mdl_builtin_types.h
src/mdl_builtin_types.h: src/mdl_builtin_types.cpp ;
src/mdl_builtins.h: src/mdl_builtins.cpp ;

src/mdl_builtin_types.cpp: src/macros.hpp src/mdl_builtins.h
	$(PERL) scripts/make_types.pl $(@:.cpp=.h) < $< > $@	

src/macros.o: src/mdl_builtin_types.h src/mdl_builtins.h src/mdl_internal_defs.h
src/mdl_output.o: src/mdl_builtin_types.h src/mdl_builtins.h src/mdl_internal_defs.h
src/mdl_read.o: src/mdl_builtin_types.h src/mdl_builtins.h src/mdl_internal_defs.h
src/mdl_binary_io.o: src/mdl_builtin_types.h src/mdl_builtins.h src/mdl_internal_defs.h

clean: 
	rm -f src/*.o *~ $(PROGRAMS)

extraclean: clean
	rm -f $(TMPSRCS)
