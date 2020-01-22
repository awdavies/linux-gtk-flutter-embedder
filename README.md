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

