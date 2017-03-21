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

all: $(BINARY)
	
$(BINARY): libxpcsc $(OBJS)
	g++ -g -o $@ $(OBJS) $(CPPFLAGS) $(LDFLAGS)

libxpcsc:
	cd ../libxpcsc && make

clean:
	rm -f $(BINARY) $(OBJS)

%.o: %.cpp %.hpp
	g++ -Wall -c -o $@ $< $(CPPFLAGS)

.PHONY: libxpcsc
