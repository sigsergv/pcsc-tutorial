all: libxpcsc.a

libxpcsc.a:
	cd src && make all
	cp src/libxpcsc.a ./

clean:
	rm -f libxpcsc.a
	cd src && make clean

.PHONY: libxpcsc.a