#-----Macros---------------------------------

INCLUDES = -I.

# set up compiler and options
CC = gcc
CFLAGS = -g -Wall $(INCLUDES)
# Explain -g and -Wall and require -Wall and no warnings on submissions

#-----Suffix Rules---------------------------
# set up C suffixes and relationship between .c and .o files

.SUFFIXES: .c

.c.o:
	$(CC) $(CFLAGS) -c $<

.c :
	$(CC) $(CFLAGS) $< -o $@

#-----File Dependencies----------------------

SRC = client.c server.c

OBJ = client.o server.o

all: client server

client: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

server: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

#-----Other stuff----------------------------
depend:
	makedepend $(CFLAGS) -Y $(SRC)

clean:
	rm -f $(OBJ) client
	rm -f $(OBJ) server
