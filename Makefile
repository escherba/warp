.PHONY: clean

warp: warp.cpp
	g++ -o warp `pkg-config --cflags --libs opencv` warp.cpp

clean:
	rm -f warp
