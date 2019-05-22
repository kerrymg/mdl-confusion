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
CFLAGS = -Isrc -Ibuild $(CDEBUGFLAGS) $(COPTFLAGS) $(CWARNFLAGS)
CXXFLAGS = -std=c++11 -Isrc -Ibuild $(CDEBUGFLAGS) $(COPTFLAGS) $(CWARNFLAGS)

PERL = perl

PROGRAMS = mdli

TMPSRCS = build/mdl_builtins.cpp \
	build/mdl_builtin_types.cpp \
	build/mdl_builtin_types.h \
	build/mdl_builtins.h \
	build/license.c

CXXSRCS = src/macros.cpp \
	src/mdli.cpp \
	build/mdl_builtins.cpp \
	build/mdl_builtin_types.cpp \
	src/mdl_read.cpp \
	src/mdl_output.cpp \
	src/mdl_binary_io.cpp \
	src/mdl_decl.cpp \
	src/mdl_assoc.cpp

CSRCS = src/mdl_strbuf.c build/license.c

# There's probably a better way of doing this...
BUILDCXX = $(patsubst src/%.cpp,build/%.cpp,$(CXXSRCS))
BUILDC = $(patsubst src/%.c,build/%.c,$(CSRCS))
OBJS = $(BUILDCXX:.cpp=.o) $(BUILDC:.c=.o)

mdli: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

build/license.c: LICENSE
	awk 'BEGIN { print "const char license [] = " } /END OF TERMS AND CONDITIONS/ { nextfile } { gsub("\"", "\\\"", $$0); print "\"" $$0 "\\n\""  } END { print ";" }'  < LICENSE > $@

build/mdl_builtins.o: build/mdl_builtins.h

build/mdl_builtins.cpp: src/macros.cpp
	@mkdir -p build
	$(PERL) scripts/find_builtins.pl $(@:.cpp=.h) < $< > $@	

build/mdl_builtin_types.o: build/mdl_builtin_types.h
build/mdl_builtin_types.h: build/mdl_builtin_types.cpp ;
build/mdl_builtins.h: build/mdl_builtins.cpp ;

build/mdl_builtin_types.cpp: src/macros.hpp build/mdl_builtins.h
	@mkdir -p build
	$(PERL) scripts/make_types.pl $(@:.cpp=.h) < $< > $@	

build/macros.o: build/mdl_builtin_types.h build/mdl_builtins.h src/mdl_internal_defs.h
build/mdl_output.o: build/mdl_builtin_types.h build/mdl_builtins.h src/mdl_internal_defs.h
build/mdl_read.o: build/mdl_builtin_types.h build/mdl_builtins.h src/mdl_internal_defs.h
build/mdl_binary_io.o: build/mdl_builtin_types.h build/mdl_builtins.h src/mdl_internal_defs.h

build/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean: 
	rm -f build/*.o *~ $(PROGRAMS)

extraclean: clean
	rm -rf build
