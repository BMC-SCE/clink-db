clink-db - a fork of clink a re-implementation of Cscope
=========================================================

*this is currently a work in progress and should not be considered stable*

When working in a large, complex C/C++ code base, an invaluable navigation tool
is Cscope. However, Cscope is showing its age and has some issues that prevent
it from being the perfect assistant. clink-db aims to be bring the Cscope
experience into the twenty first century.

What does that mean?
--------------------

* **Full C/C++ semantic support** clink-db uses libclang, which means it
  understands your code (including macros) as well as your compiler does.
* **Multicore support** Parsing multiple, independent files trivially
  parallelises, so why be limited by one core?
* **Assembly support** Systems code often calls into assembly, at which point
  Cscope gets lost. clink-db parses assembly fuzzily and can maintain a call
  graph across this boundary.
* **Syntax highlighting** You’re probably used to looking at code in Vim with
  syntax highlighting, so clink-db can ask Vim to highlight the snippets it shows
  you.
* **Exact jumps** clink-db opens Vim not only at the right line, but at the right
  column for the entry you’ve asked for.
* **Fewer features** Cscope’s options to find files and regex text are now
  better served by any number of other tools and are not included in clind-db.

Building clink-db
-----------------
```
  # Centos 8 dependencies:
  dnf install clang clang-libs clang-devel llvm llvm-libs llvm-devel sqlite sqlite-libs sqlite-devel

  # download clink-db
  git clone -b dev-c https://github.com/BMC-SCE/clink-db.git
  cd clink-db

  # configure and compile
  mkdir build
  cd build
  cmake ..
  cmake --build .
```

Files extensions
----------------

C/C++ - c,c++,cpp,cxx,cc,h,hpp
ASM   - s

Notes for devs
--------------

* The on-disk database format is extremely bloated. As with performance, I have
  put no effort into optimising this yet and easy wins abound.
* Vim integration is currently hard coded. I didn’t make this parametric or
  implement any abstraction for this because Vim is my unabashed weapon of
  choice. If you want support for another editor, please ask me and I’ll
  probably do it.
* The line-oriented interface is a bare bones hack intended to mimic Cscope just
  well enough to fool Vim into talking to it. If you want a more full featured
  line-oriented interface, please ask.
* Cscope’s "find assignments to this symbol" is not implemented. Honestly, I
  have never used this query. Have you? It actually sounds really useful, but I
  have never once thought of this until enumerating Cscope’s options while
  implementing clink-db.
* Some open questions about Cscope that I haven’t yet explored:

  * Why do Cscope’s line-oriented and curses interface results differ? The
    example I have on hand is a file that #includes a file from a parent
    directory. If I had to speculate, I’d say this is actually a Cscope bug.
    The discrepancy seems to lead to a worse user experience in Cscope (file
    jumping in Vim that doesn’t work) though admittedly I’ve never actually
    noticed this until staring at Cscope results while implementing clink-db.

Anything else you don’t understand, ask away. Questions are the only way I
learned enough to write this thing.

Legal
-----
Everything in this repository is in the public domain, under the terms of
the Unlicense. For the full text, see LICENSE.
