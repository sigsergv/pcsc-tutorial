
BINARY := atr-parse
OBJS := atr-parse.o pcsc_context.o bytes_list.o

CPPFLAGS := 
LDFLAGS := 

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CPPFLAGS += -I/usr/include/PCSC
	LDFLAGS += -lpcsclite
endif
ifeq ($(UNAME_S),Darwin)
	CPPFLAGS +=
	LDFLAGS += -framework PCSC
endif

all: $(BINARY)
	
$(BINARY): $(OBJS)
	g++ -g -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -f $(BINARY) $(OBJS)

%.o: %.cpp
	g++ -Wall -c -o $@ $< $(CPPFLAGS)
