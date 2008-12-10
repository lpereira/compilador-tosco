CFLAGS = -g -O3 -Wall -pipe `pkg-config glib-2.0 --cflags` `pkg-config libglade-2.0 --cflags` `pkg-config gtk+-2.0 --cflags`
LIBS = `pkg-config glib-2.0 --libs` `pkg-config gtk+-2.0 --libs` `pkg-config libglade-2.0 --libs`
OBJECTS = mvd_glade.o ui.o vm.o treeview.o main.o

all:
	@make pixmaps
	@make mvd

pixmaps:
	gdk-pixbuf-csource --raw --name=arrow arrow.png > arrow.h
	gdk-pixbuf-csource --raw --name=gear gear.png > gear.h

mvd_glade.o:
	./blob-to-object mvd.glade

mvd:	$(OBJECTS)
	$(CC) $(CFLAGS) -o mvd $(OBJECTS) $(LIBS)

clean:
	rm -f *.o *~ mvd_glade.h arrow.h mvd
