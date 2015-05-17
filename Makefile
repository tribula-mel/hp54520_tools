# Makefile for the HP545xx oscilloscope software
# Copyright (C) 2015 Tribula Mel <tribula.mel@gmail.com> 
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

CC=gcc
CFLAGS=-c -Wall -g
LDFLAGS=
SOFLAFS =
SHARED = 
LDSHARED =

all: lif_convert extract_sections extract_subsection recreate_wsfile \
     generate_lif

lif_convert: lif_convert.o
	$(CC) lif_convert.o -o lif_convert $(LDFLAGS)

lif_convert.o: lif_convert.c
	$(CC) $(CFLAGS) lif_convert.c

extract_sections: extract_sections.o
	$(CC) extract_sections.o -o extract_sections $(LDFLAGS)

extract_sections.o: extract_sections.c
	$(CC) $(CFLAGS) extract_sections.c

extract_subsection: extract_subsection.o
	$(CC) extract_subsection.o -o extract_subsection $(LDFLAGS)

extract_subsection.o: extract_subsection.c
	$(CC) $(CFLAGS) extract_subsection.c

recreate_wsfile: recreate_wsfile.o
	$(CC) recreate_wsfile.o -o recreate_wsfile $(LDFLAGS)

recreate_wsfile.o: recreate_wsfile.c
	$(CC) $(CFLAGS) recreate_wsfile.c

generate_lif: generate_lif.o
	$(CC) generate_lif.o -o generate_lif $(LDFLAGS)

generate_lif.o: generate_lif.c
	$(CC) $(CFLAGS) generate_lif.c

clean:
	rm -rf core cscope.* *.o lif_convert extract_sections \
	extract_subsection recreate_wsfile generate_lif

