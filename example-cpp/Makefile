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

all: read-atr
	
read-atr: libxpcsc read-atr.o
	g++ -g -o $@ read-atr.o $(CPPFLAGS) $(LDFLAGS)

libxpcsc:
	cd ../libxpcsc && make

clean:
	rm -f $(BINARY) $(OBJS)

%.o: %.cpp
	g++ -Wall -c -o $@ $< $(CPPFLAGS)

.PHONY: libxpcsc
