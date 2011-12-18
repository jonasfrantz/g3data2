CC=gcc
CFLAGS=-Wall `pkg-config --cflags gtk+-3.0` -DG_DISABLE_DEPRECATED=1 -DGDK_DISABLE_DEPRECATED=1 -DGDK_PIXBUF_DISABLE_DEPRECATED=1 -DGTK_DISABLE_DEPRECATED=1
LIBS=`pkg-config --libs gtk+-3.0`
bindir ?= /usr/bin
mandir ?= /usr/share/man

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

all: main.o sort.o points.o drawing.o g3data2.1.gz
	$(CC) $(CFLAGS) -o g3data2 main.o sort.o points.o drawing.o $(LIBS)
	strip g3data2

main.o: main.c main.h strings.h vardefs.h

sort.o: sort.c main.h

points.o: points.c main.h

drawing.o: drawing.c main.h

g3data2.1.gz: g3data2.sgml
	rm -f *.1
	onsgmls g3data2.sgml | sgmlspl /usr/share/sgml/docbook/utils-0.6.14/helpers/docbook2man-spec.pl
	gzip g3data2.1

clean:
	rm -f *.o g3data2 g3data2.1.gz *~ manpage.*

install:
	install g3data2 $(bindir)
	install g3data2.1.gz $(mandir)/man1

uninstall:
	rm $(bindir)/g3data2
