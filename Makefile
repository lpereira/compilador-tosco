CFLAGS = -g -O0 -Werror -Wall -pipe `pkg-config glib-2.0 --cflags`
LIBS = `pkg-config glib-2.0 --libs`
OBJECTS = stack.o symbol-table.o lex.o ast.o codegen.o charbuf.o \
		tokenlist.o optimization-l1.o optimization-l2.o \
		codeemitter.o \
		writer-c.o writer-tac.o writer-stack.o \
		main.o

all:	$(OBJECTS)
	$(CC) $(CFLAGS) -o compiler $(OBJECTS) $(LIBS)

clean:
	rm -f *.o *~
	rm -f compiler

