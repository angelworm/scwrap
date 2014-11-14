PROGRAMS = scwrap
scwrap_SOURCES = scwrap.c
scwrap_LDFLAGS = -framework CoreFoundation -framework CoreServices -framework SystemConfiguration
scwrap_CFLAGS = 

all: $(PROGRAMS)

scwrap: $(scwrap_SOURCES:.c=.o)
	$(CC) $(scwrap_LDFLAGS) -o $@ $<

clean:
	-rm $(PROGRAMS)
	-rm *.o

