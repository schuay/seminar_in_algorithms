all: build

build:
	mkdir -p build
	cd build; cmake -DCMAKE_BUILD_TYPE="Release" ..; make -j6

clean:
	rm -rf build

.PHONY: build
