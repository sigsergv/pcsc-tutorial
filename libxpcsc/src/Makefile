UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    CPPFLAGS += -I/usr/include/PCSC -I../include
    # LDFLAGS += -lpcsclite
endif
ifeq ($(UNAME_S),Darwin)
    CPPFLAGS +=
    # LDFLAGS += -framework PCSC
endif

ifneq ($(DEBUG),)
	CPPFLAGS += -DDEBUG
endif

all: libxpcsc.a

clean:
	rm -f *.a *.o

libxpcsc.a: connection.o exceptions.o format.o access_bits.o atrparser.o
	ar -rcs $@ $^

%.o: %.cpp ../include/xpcsc.hpp
	g++ -Wall -c $< $(CPPFLAGS) -o $@

# connection.o: connection.cpp ../include/xpcsc.hpp 
# 	g++ -Wall -c -o $@ connection.cpp $(CPPFLAGS)

.PHONY: clean