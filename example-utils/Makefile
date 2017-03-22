# this file should be included to example-XX subprojects
CPPFLAGS := -I../libxpcsc/include
LDFLAGS := ../libxpcsc/libxpcsc.a

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CPPFLAGS += -I/usr/include/PCSC
	LDFLAGS += -lpcsclite
endif
ifeq ($(UNAME_S),Darwin)
	CPPFLAGS +=
	LDFLAGS += -framework PCSC
endif

SIMPLE_BINARIES := dump-mifare-card dump-atr

all: libxpcsc $(SIMPLE_BINARIES)


libxpcsc:
	cd ../libxpcsc && make
	
$(SIMPLE_BINARIES): %: %.o
	g++ -g -o $@ $@.o  $(CPPFLAGS) $(LDFLAGS) 

clean:
	rm -f $(BINARY) $(OBJS)

%.o: %.cpp
	g++ -Wall -c -o $@ $< $(CPPFLAGS)

.PHONY: libxpcsc
