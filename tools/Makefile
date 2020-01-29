all: screw

screw: screw.c zencode.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o screw screw.c zencode.c -lz

clean:
	$(RM) *.o *.so screw
