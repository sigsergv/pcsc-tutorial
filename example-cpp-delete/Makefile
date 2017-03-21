
BINARIES := scan_atr scan_atr_file test_atr_parser
COMMON_OBJS := pcsc_context.o bytes_list.o atr_parser.o

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

ifneq ($(DEBUG),)
	CPPFLAGS += -DDEBUG
endif

all: $(BINARIES)
	
scan_atr: scan_atr.o $(COMMON_OBJS)
	g++ -g -o $@ $^ $(LDFLAGS)

scan_atr_file: scan_atr_file.o $(COMMON_OBJS)
	g++ -g -o $@ $^ $(LDFLAGS)

test_atr_parser: test_atr_parser.o $(COMMON_OBJS)
	g++ -g -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(BINARIES) *.o

%.o: %.cpp %.h
	g++ -Wall -c -o $@ $< $(CPPFLAGS)
