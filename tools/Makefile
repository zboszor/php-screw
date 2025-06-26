all: screw unscrew

screw: screw.c ../zencode.c
	$(CC) $(CFLAGS) $(LDFLAGS) -I.. -DSCREW_TOOL=1 -o $@ $^ -lz

unscrew: unscrew.c ../zencode.c
	$(CC) $(CFLAGS) $(LDFLAGS) -I.. -DSCREW_TOOL=1 -o $@ $^ -lz

clean:
	$(RM) *.o *.so screw unscrew
