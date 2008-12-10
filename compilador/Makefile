CFLAGS = -g -O3 -Wall  -pipe `pkg-config glib-2.0 --cflags` `pkg-config gtksourceview-2.0 --cflags` `pkg-config libglade-2.0 --cflags` `pkg-config gtk+-2.0 --cflags`
LIBS = `pkg-config glib-2.0 --libs` `pkg-config gtk+-2.0 --libs` `pkg-config libglade-2.0 --libs` `pkg-config gtksourceview-2.0 --libs`
OBJECTS = lpd_lang.o compiler_glade.o ui.o gui_main.o \
 	  stack.o symbol-table.o lex.o ast.o codegen.o charbuf.o \
	  tokenlist.o optimization.o \
	  compiler_main.o treeview.o conf.o \
	  main.o

all:	
	@make hammer
	@make csd

hammer:
	gdk-pixbuf-csource --rle --name=hammer hammer.png > hammer.h

lpd_lang.o:
	./blob-to-object lpd.lang
	
compiler_glade.o:
	./blob-to-object compiler.glade

csd:	$(OBJECTS)
	$(CC) $(CFLAGS) -o csd $(OBJECTS) $(LIBS)

update-glade:
	rm -f compiler_glade.o ui.o
	make all

clean:
	rm -f *.o *~ compiler_glade.h lpd_lang.h csd

dox:
	doxygen
	