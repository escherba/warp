.PHONY: clean debug

warp: warp.cpp
	g++ -o warp `pkg-config --cflags --libs opencv` warp.cpp

debug:
	g++ -g -o warp `pkg-config --cflags --libs opencv` warp.cpp

clean:
	rm -f warp
