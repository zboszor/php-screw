all: screw

screw: screw.c ../zencode.c
	$(CC) $(CFLAGS) $(LDFLAGS) -I.. -DSCREW_TOOL=1 -o screw screw.c ../zencode.c -lz

clean:
	$(RM) *.o *.so screw
