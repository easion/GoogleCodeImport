OBJECTS=card.o freecell.o option.o stack.o subwindows.o undo.o util.o gnmanager.o random.o
CC=g++
xCFLAGS=-g -Wall -DSHAPE -DBOGUSRANDOM
CFLAGS=-g -Wall -DBOGUSRANDOM `pkg-config --cflags gix`
xLIBS=-lm -L. -lns -L/usr/X11R6/lib -lXext -lX11
LIBS=-lm -L. -lns `pkg-config --libs gix`
STATICDIR=xfreecell-static
DOCS=README CHANGES mshuffle.txt xfreecell.6
LIBDIR=/usr/local/lib/xfreecell
export CFLAGS LIBS

all: $(OBJECTS) lib
	$(CC) -o xfreecell $(CFLAGS) $(OBJECTS) $(LIBS)

static: $(OBJECTS) lib
	$(CC) -o xfreecell -static $(CFLAGS) $(OBJECTS) $(LIBS)

static-release: static
	mkdir $(STATICDIR)
	strip xfreecell
	mv xfreecell $(STATICDIR)
	cp $(DOCS) $(STATICDIR)
	tar czf xfreecell-bin.tgz $(STATICDIR)
	rm -rf $(STATICDIR)

%.o: %.cpp
	$(CC) -c $(CFLAGS) $<

clean:
	rm -f *~ *.o a.out xfreecell libns.a
	make -C widget clean

lib:
	make -C widget lib

install: all
	install xfreecell $(DESTDIR)/usr/local/bin
	install xfreecell.6 $(DESTDIR)/usr/local/man/man6
#	install -d $(LIBDIR)
#	install ms-compatible/MSNumbers $(DESTDIR)/usr/local/lib/xfreecell
