UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
	CCFLAGS +=
	LDFLAGS +=
endif
ifeq ($(UNAME_S),Darwin)
	CCFLAGS +=
	LDFLAGS += -framework PCSC
endif

example-01: example-01.o
	$(CC) -o $@ $^ $(CCFLAGS) $(LDFLAGS)

clean:
	rm -f *.o example-01

%.o: %.c
	$(CC) -c -o $@ $< $(CCFLAGS)
