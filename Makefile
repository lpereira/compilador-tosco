CFLAGS = -g -O0 -Wall -pipe `pkg-config glib-2.0 --cflags`
LIBS = `pkg-config glib-2.0 --libs`
OBJECTS = stack.o symbol-table.o lex.o ast.o codegen.o charbuf.o \
		tokenlist.o optimization-l1.o main.o codeemitter.o \
		writer-tac.o writer-risclie.o writer-llvm.o \
		optimization-l2.o

all:	$(OBJECTS)
	$(CC) $(CFLAGS) -o compiler $(OBJECTS) $(LIBS)

clean:
	rm -f *.o *~
	rm -f compiler

