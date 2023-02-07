CC            = ia16-elf-gcc
CFLAGS        = -march=i8086 -mtune=i8086 -mcmodel=small -fexec-charset=CP932
LIBS          = -li86
OBJS          = iopm.o
PROGRAM       = iopm.exe

all:		$(PROGRAM)

$(PROGRAM):	$(OBJS)
		$(CC) $(OBJS) $(LIBS) -o $(PROGRAM)

clean:		
		rm -f *.o *~ $(PROGRAM)
		rm -f -r docs

docs:		
		doxygen
