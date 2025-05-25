C_SRC := $(wildcard *.c)
S_SRC := $(wildcard *.S)
OBJ := $(C_SRC:.c=.o) $(S_SRC:.S=.o)

.PHONY: all clean

all: $(OBJ)

clean:
	rm -f *.o

%.o: %.c
	$(GCC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -c $<
%.o: %.S
	$(GCC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -c $<