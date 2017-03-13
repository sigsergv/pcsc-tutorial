OBJS := print_bytes.o
BINARY := libutil.a

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CCFLAGS += -I/usr/include/PCSC
	LDFLAGS += -lpcsclite
endif
ifeq ($(UNAME_S),Darwin)
	CCFLAGS +=
	LDFLAGS += -framework PCSC
endif

$(BINARY): $(OBJS)
	ar -rcs $@ $^

clean:
	rm -f $(BINARY) $(OBJS)

%.o: %.c
	$(CC) -Wall -c -o $@ $< $(CCFLAGS)
