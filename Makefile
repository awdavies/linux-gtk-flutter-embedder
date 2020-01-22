FLUTTER_ENGINE_LIB=flutter_engine
CXX=g++ -std=c++14
CXXFLAGS=-Wall -Werror $(shell pkg-config --cflags gtk+-3.0 x11 epoxy)
LDFLAGS=-L$(CURDIR) \
	$(shell pkg-config --libs gtk+-3.0 x11 epoxy) \
	-l$(FLUTTER_ENGINE_LIB) \
	-Wl,-rpath=$(CURDIR)

CC_FILES=$(wildcard *.cc)
HEADERS=$(wildcard include/*.h)
SOURCES=$(HEADERS) $(CC_FILES)

all: flutter_embedder

flutter_embedder: $(SOURCES) lib$(FLUTTER_ENGINE_LIB).so
	$(CXX) $(CXXFLAGS) $(CC_FILES) $(LDFLAGS) -o $@

.PHONY: clean
clean:
	rm -f flutter_embedder
