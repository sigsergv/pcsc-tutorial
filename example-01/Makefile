OBJS := example-01.o
BINARY := example-01

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
	$(CC) -o $@ $^ $(CCFLAGS) $(LDFLAGS)

clean:
	rm -f $(BINARY) $(OBJS)

%.o: %.c
	$(CC) -Wall -c -o $@ $< $(CCFLAGS)
