.PHONY: clean

warp: warp.cpp
	g++ -o warp warp.cpp `pkg-config --cflags --libs opencv` 

debug: warp.cpp
	g++ -g -o warp  warp.cpp `pkg-config --cflags --libs opencv`

clean:
	rm -f warp

