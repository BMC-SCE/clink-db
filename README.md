clink-db = a fork of clink (https://github.com/Smattr/clink) a re-implementation of cscope
==========================================================================================

When working in a large, complex C/C++ code base, an invaluable navigation tool
is cscope. However, cscope is showing its age and has some issues that prevent
it from being the perfect assistant. clink-db aims to be bring the cscope
experience into the twenty first century.

What does that mean?
--------------------

* **Full C/C++ semantic support** clink-db uses clang/libclang, which means it
  understands your code (including macros) as well as your compiler does.
* **Multicore support** Parsing multiple, independent files trivially
  parallelises, so why be limited by one core?
* **Assembly support** Systems code often calls into assembly, at which point
  cscope gets lost. clink-db parses assembly fuzzily and can maintain a call
  graph across this boundary.
* **Syntax highlighting** You’re probably used to looking at code in Vim with
  syntax highlighting, so clink-db can ask Vim to highlight the snippets it shows
  you.
* **Exact jumps** clink-db opens Vim not only at the right line, but at the right
  column for the entry you’ve asked for.
* **Fewer features** cscope’s options to find files and regex text are now
  better served by any number of other tools and are not included in clind-db.

Building clink-db
-----------------
```
  # Centos 8 dependencies:
  dnf install clang clang-libs clang-devel llvm llvm-libs llvm-devel sqlite sqlite-libs sqlite-devel file file-libs file-devel
  
  # modify libdir, includedir if they have different paths on your OS and copy in pkg-config directory
  cp libmagic.pc /usr/lib64/pkgconfig/libmagic.pc

  # download clink-db
  git clone -b master https://github.com/BMC-SCE/clink-db.git
  # or
  git clone -b dev https://github.com/BMC-SCE/clink-db.git
  cd clink-db

  # configure and compile
  mkdir build
  cd build
  cmake ..
  cmake --build .
```

Differences from the original version
--------------------------------------
       --full-ast, -a
              Uses clang/clang++ for emit AST for source inputs, by default it uses libclang which does not generate full AST.

       --all-files
              All  files  are treated as C/C++, it tries with both clang/clang++ to emit AST, useful when there are source files with unusual extensions and libmagic does not detect
              their content. Valid only with --full-ast option.

       --help-cx, -H
              Display CXCursorKind information.

       --kernel, -k
              "Kernel Mode", turns off the use of the default include dir (usually /usr/include) when building the database, since kernel source trees generally do not use it.

       --skip-vim, -S
              Does not insert data in tables content and records, useful for fast symbol processing.

       --language, -x TYPE
              Treat  subsequent  input  files  as  having TYPE C or C++. Clink-db uses libmagic by default when used with the -a option to detect the file type. This option sets the
              CLINK_CC environment variable to clang or clang++.

       --cflags FILE, -C FILE
              Give additional arguments to clang (similar with make CFLAGS) when used with the -a option.  All arguments must be on a single line separated by spaces.

       --cxxflags FILE, -X FILE
              Give additional arguments to clang++ (similar with make CXXFLAGS) when used with the -a option.  All arguments must be on a single line separated by spaces.

clink-db generates the following files:
clink-db.log
clang-parse.log
clink-db.sqlite3

Without libmagic files extensions
---------------------------------

C/C++ - c, c++, cpp, cxx, cc, h, hpp  
ASM   - s, asm

Original notes for devs
-----------------------

* The on-disk database format is extremely bloated.
* Vim integration is currently hard coded. I didn’t make this parametric or
  implement any abstraction for this because Vim is my unabashed weapon of
  choice. If you want support for another editor, please ask me and I’ll
  probably do it.
* The line-oriented interface is a bare bones hack intended to mimic cscope just
  well enough to fool Vim into talking to it. If you want a more full featured
  line-oriented interface, please ask.
* cscope’s "find assignments to this symbol" is not implemented. Honestly, I
  have never used this query. Have you? It actually sounds really useful, but I
  have never once thought of this until enumerating cscope’s options while
  implementing clink-db.
* Some open questions about cscope that I haven’t yet explored:

  * Why do cscope’s line-oriented and curses interface results differ? The
    example I have on hand is a file that #includes a file from a parent
    directory. If I had to speculate, I’d say this is actually a cscope bug.
    The discrepancy seems to lead to a worse user experience in cscope (file
    jumping in Vim that doesn’t work) though admittedly I’ve never actually
    noticed this until staring at cscope results while implementing clink-db.

Anything else you don’t understand, ask away. Questions are the only way I
learned enough to write this thing.

Legal
-----
Everything in this repository is in the public domain, under the terms of
the Unlicense. For the full text, see LICENSE.
