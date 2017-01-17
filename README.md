# Clink -- a modern re-implementation of Cscope

**_this is currently a work in progress and should not be considered stable_**

When working in a large, complex C/C++ code base, an invaluable navigation tool
is [Cscope](http://cscope.sourceforge.net/). However, Cscope is showing its age
and has some issues that prevent it from being the perfect assistant. Clink aims
to be bring the Cscope experience into the twenty first century.

## What does that mean?

  * **Full C/C++ semantic support** Clink uses libclang, which means it
    understands your code (including macros) as well as your compiler does.
  * **Multicore support** Parsing multiple, independent files trivially
    parallelises, so why be limited by one core?
  * **_TBC: not implemented yet_** **Assembly support** Systems code often calls
    into assembly, at which point Cscope gets lost. Clink parses assembly
    fuzzily and can maintain a call graph across this boundary.
  * **_TBC: not implemented yet_** **Syntax highlighting** You're probably used
    to looking at code in Vim with syntax highlighting, so Clink can ask Vim to
    highlight the snippets it shows you.
  * **Exact jumps** Clink opens Vim not only at the right line, but at the right
    column for the entry you've asked for.
  * **Fewer features** Cscope's options to find files and regex text are now
    better served by [any](http://blog.burntsushi.net/ripgrep/)
    [number](http://geoff.greer.fm/ag/) [of](http://beyondgrep.com/)
    [other](https://en.wikipedia.org/wiki/Grep)
    [tools](https://en.wikipedia.org/wiki/Sed) and are not included in Clink.

## Notes for devs

  * If you've tried using this, you will note it is extremely slow currently.
    Almost no effort has been put into optimisation yet and there is likely an
    abundance of low hanging fruit. I'm focusing on getting feature complete
    first.
  * If Clink is modern, its build system is decidedly ancient. I haven't spent
    the time to divine a CMakeFiles.txt that can find libclang, but it is on my
    todo list.
  * The on-disk database format is extremely bloated. As with performance, I
    have put no effort into optimising this yet and easy wins abound.
  * Vim integration is currently hard coded. I didn't make this parametric
    or implement any abstraction for this because Vim is my unabashed weapon of
    choice. If you want support for another editor, please ask me and I'll
    probably do it.
  * The line-oriented interface is a bare bones hack intended to mimic Cscope
    just well enough to fool Vim into talking to it. If you want a more full
    featured line-oriented interface, please ask.
  * Cscope's "find assignments to this symbol" is not implemented. Honestly, I
    have never used this query. Have you? It actually sounds really useful, but
    I have never once thought of this until enumerating Cscope's options while
    implementing Clink.
  * An inevitable question is why I didn't just modify Cscope and push the
    changes upstream. However, I think it's pretty clear that Clink takes a
    different (and incompatible) path to upstream. Pulling in libclang as a
    dependency is unacceptable for many constrained environments where Cscope
    needs to run. I myself have used Cscope in many environments where libclang
    would have been an immediate deal breaker. Essentially, Cscope has no dress
    code, while Clink expects you to have plenty of cores and RAM for days.
  * Some open questions about Cscope that I haven't yet explored:
    * Why do Cscope's line-oriented and curses interface results differ? The
      example I have on hand is a file that #includes a file from a parent
      directory. If I had to speculate, I'd say this is actually a Cscope bug.
      The discrepancy seems to lead to a worse user experience in Cscope (file
      jumping in Vim that doesn't work) though admittedly I've never actually
      noticed this until staring at Cscope results while implementing Clink.
    * Why does Cscope open Vim at the right line, but at the first column? It
      (indirectly) has the column information as well, so why not take advantage
      of it?
    * Why does Cscope display results with differing white space from the source
      file? It has the exact white space information, doesn't it?

Anything else you don't understand, ask away. Questions are the only way I
learned enough to write this thing.

## Legal

Play nice. If you want to do something big or something commercial with this
code, please ask. Chances are I'll say yes. If you want to hack on this for fun
or personal use, go ahead.
