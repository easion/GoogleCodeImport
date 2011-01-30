xOBJECTS=window.o plate.o string.o button.o label.o textfield.o container.o frame.o util.o scrollbar.o font.o main.o
OBJECTS=window.o plate.o string.o button.o label.o textfield.o container.o frame.o util.o font.o main.o
#C=/usr/local/gcc-2.8.1/bin/g++
CC=g++
xCFLAGS=-g -Wall
xLIBS=-L/usr/X11R6/lib -lX11
LIBNAME=libns.a

all: test.o $(OBJECTS)
	$(CC) $(CFLAGS) test.o $(OBJECTS) $(LIBS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *~ a.out .nfs* dummy

tgz:
	tar cvzf widget.tgz *.cpp *.h makefile

dummy: $(OBJECTS)
	ar r $(LIBNAME) $(OBJECTS)
	ranlib $(LIBNAME)
	mv $(LIBNAME) ..
	touch dummy

lib: $(OBJECTS) dummy
