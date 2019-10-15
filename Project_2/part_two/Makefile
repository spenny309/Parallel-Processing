######################################################################
#
# 'make'         : create program $(PROG)
# 'make clean'   : clean directory
#
######################################################################

# list your source/assembly files in SRCS
SRCS = cs338-prog2.c

CFLAGS = -I.
LDLIBS = -L.

PROG = prog2

.PHONY: default clean

default: $(PROG)

$(PROG): $(SRCS) 
	gcc $(CFLAGS) $(LDLIBS) -o $@ $^  -lpthread -ljpeg

$(PROG).o: 
	gcc $(CFLAGS) -o $@ -c $^

clean:
	rm -f *~ $(DRIVER).o $(PROG) $(PROG).o $(PROGEX)


