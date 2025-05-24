ASM_SRC := $(wildcard *.S)
C_SRC := $(wildcard *.c)
OBJ := $(ASM_SRC:.S=.o) $(C_SRC:.c=.o)

.PHONY: all clean

all: $(OBJ)

clean:
	rm -f *.o *.Identifier *.d

%.o: %.S
	$(GCC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -c $<
%.o: %.c
	$(GCC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -c $<

-include $(DEPS)