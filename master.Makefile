# this file should be included to example-XX subprojects
CCFLAGS := 
LDFLAGS := ../util/libutil.a

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CCFLAGS += -I/usr/include/PCSC
	LDFLAGS += -lpcsclite
endif
ifeq ($(UNAME_S),Darwin)
	CCFLAGS +=
	LDFLAGS += -framework PCSC
endif

all: $(BINARY)
	
$(BINARY): libutil $(OBJS)
	$(CC) -o $@ $(OBJS) $(CCFLAGS) $(LDFLAGS)

libutil:
	cd ../util && make

clean:
	rm -f $(BINARY) $(OBJS)

%.o: %.c
	$(CC) -Wall -c -o $@ $< $(CCFLAGS)

.PHONY: libutil
