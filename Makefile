CC=gcc
LDFLAGS=-lreadline

OUTPUT=tsh
CFILE=shell.c

default: $(OUTPUT)

$(OUTPUT): $(CFILE) libtalaris.a
	gcc $(CFLAGS) $^  -o $(OUTPUT) $(LDFLAGS)

clean:
	trash *.o *.a
