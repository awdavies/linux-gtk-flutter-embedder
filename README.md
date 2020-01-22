# Dependencies

Needs:

*   `pkg-config`
*   `gtk-3`
*   `freeglut-dev`
*   `flutter-engine` (more on this below)

# Building and Running the Embedder

1) Make sure to build the latest version of the Flutter Engine (steps for
building the Engine can be found
[here](https://github.com/flutter/engine/blob/master/CONTRIBUTING.md).
2) Copy `libflutter_engine.so` into the root Linux embedder directory.
3) Run `make`
4) Run ./main

# State of the repo.

This was mostly an exploratory effort. Note that it contains many hacks that
might not be necessary (mutexes, fences, etc). If you'd like more info please
feel free to open an issue and I can attempt to upload older, potentially less
hacky versions of the code.
