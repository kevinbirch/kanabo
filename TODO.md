-*- gfm -*-

#  金棒 (kanabō)

## TODO

### global

* jit compiler for evaluator?
  * http://eli.thegreenplace.net/2013/10/17/getting-started-with-libjit-part-1/
  * http://www.stephendiehl.com/llvm/
* https://www.securecoding.cert.org/confluence/display/seccode/CERT+C+Secure+Coding+Standard
* valgrind
* frama-c
* memory optimizations
  * http://blog.libtorrent.org/2013/12/memory-cache-optimizations/
  * http://www.reddit.com/r/programming/comments/1u660a/the_lost_art_of_c_structure_packing/
  * http://linux.die.net/man/1/pahole
  * https://github.com/arvidn/struct_layout
* https://code.google.com/p/american-fuzzy-lop/
* cmbc - http://www.cprover.org/cbmc/
* https://gitorious.org/linted/linted/source/8a9b2c7744af0e2d42419d0fea45c7b37b76930b:
* performance co-pilot? http://oss.sgi.com/projects/pcp/
* add completions for currently loaded model to linenoise
* add safe arithmetic functions
  * http://lists.nongnu.org/archive/html/qemu-devel/2013-01/msg05387.html
  * https://sourceware.org/ml/libc-alpha/2013-12/msg00098.html
* libbacktrace instead of execinfo?
* build: libtool integration?
* build: shtool integration?
* make 0.5 release
* switch weather example to yaml config file
* message formatting
  * http://www.tin.org/bin/man.cgi?section=3&topic=snprintf
  * http://stackoverflow.com/questions/69738/c-how-to-get-fprintf-results-as-a-stdstring-w-o-sprintf/69911#69911
* invert order of static functions?
  * http://www.reddit.com/r/C_Programming/comments/1u1ofw/is_this_code_clike/cednu4t
* various getopt alternatvies: https://news.ycombinator.com/item?id=10687375
* change license to bsd 3 clause
* break down large modules into smaller function level units
* tools comparison: https://news.ycombinator.com/item?id=11649142
* streaming mode
  * https://github.com/fizx/sit
  * http://stackoverflow.com/questions/13083491/looking-for-big-sample-dummy-json-data-file
  * https://github.com/udp/json-parser
  * https://github.com/zeMirco/sf-city-lots-json
  * https://github.com/seductiveapps/largeJSON

* update spacecadet
  * move changes back
  * add maybe, xcalloc, others?
  * print stack trace on xcalloc failure
  * mv spacecadet,linenose,yaml,check stuff to vendor

### evaluator

* refactor iteration methods to use filter, tranform, fold
* try to reuse parsers instead of creating new every time
* concrete predicate subtypes
* replace memcmp with hash compare?
* store hashcode with entry in bucket chain?
* interning for string module
  * diable macro
  * runtime enable disable
* use str in name test jsonpath model object
* computed goto dispatch table?
  * http://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables

### parser

* fix maybe usage
* must_make_regex (uses statement expr)
* check for usage of `%zd`
* x_free to dispose_x
* eliminate file headers
* eliminate double spacing
* xcalloc everywhere
* \_POSIX_C_SOURCE usage?

* merge master
* working cci build
* merge pr
* add full tracing
* model dumper w/ secret command line option, nice tree-like layout
  * `--output=ast`

### new features

* process
  * update tests
  * update model
  * update parser
  * update evaluator

1. join
1. anchor selector
1. tag selector
1. new scalar types (timestamp, etc)
1. filter
   * pratt parser for expressions
   * http://effbot.org/zone/simple-top-down-parsing.htm
   * http://www.oilshell.org/blog/2017/03/31.html
1. transformer
   * transfomer built-in functions

### loader

* can we mmap the input file and build a no-copy tree that points to strings by byte ranges?
* simplify api with maybe result
* break code into smaller files
* regex's can use '{}' repetition counts
* support sets and ordered maps
* http://cbor.io/ ?

### unit testing

* http://code.google.com/p/cmockery/ ?
* implement parameterized tests (some expression given to BEGIN_TEST that is inserted in the test boiler plate)
* add more beautiful runner output
* can ctest's style of test macros be used in check?
  * https://github.com/bvdberg/ctest
* eliminate check's end_test macro by creating hidden trampoline function that delegates to user's test function
* fuzz testing: https://en.wikipedia.org/wiki/API_Sanity_Checker
* find undefined behavior code
  * http://css.csail.mit.edu/stack/
  * clang ubsan
* http://rr-project.org/
* https://github.com/nivox/quickcheck4c ?

### documentation

* man page
  * more jsonpath details
  * interactive mode
  * pandoc?
  * ronn? http://rtomayko.github.io/ronn/ronn-format.7.html
* install file
* readme file
* static site
  * http://tinytree.info
  * sphynx?
  * jeckyl?
  * http://staticsitegenerators.net
  * site target
  * http://jashkenas.github.io/docco/
* http://www.reddit.com/r/sideprojectdemo

### build

* pkg-config - http://www.freedesktop.org/wiki/Software/pkg-config/
* http://code.google.com/p/qi-make/
* http://google-engtools.blogspot.fr/2011/08/build-in-cloud-how-build-system-works.html
* http://facebook.github.io/buck/
* http://aosabook.org/en/posa/ninja.html
* http://www.reddit.com/r/programming/comments/204pau/what_are_your_gcc_flags/
* https://bazel.build/
* gnu make 4.0
  * try --trace to check out rule ordering
* define, eval to create dynamic rules and shell?
* CI - http://about.travis-ci.org/docs/user/build-configuration/
* release step
  * https://github.com/manuelbua/gitver
* record build command
  * https://news.ycombinator.com/item?id=11228515
  * $(builddir)/compiler_flags: force mkdir -p $(builddir) echo '$(CPPFLAGS) $(CFLAGS)' | cmp -s - $@ || echo '$(CPPFLAGS) $(CFLAGS)' > $@
  * $(LIBOBJECTS) $(RTLLIBOBJECTS) $(OPTLIBOBJECTS) $(TESTOBJECTS) $(builddir)/init_qt_workdir: $(builddir)/compiler_flags
* meson - http://mesonbuild.com/
* https://bitbucket.org/scons/scons/wiki/SconsVsOtherBuildTools
* add project level language var: `C`, `C++` or `Objective-C`
  * eliminate language name directories under src
  * project level source file extension var
* build `src/vendor/*` as libs
  * per-vendor optional `CFLAGS`, `LDFLAGS`, `CC`, language, file extension
  * also `src/test-vendor`
* support multiple artifacts, fallback to assuming 1 and find main func
  * built in `main_ARTIFACT_TYPE ?= $(artifact)`, `ARTIFACTS ?= main_ARTIFACT`
* multi target projects
  * assume 1
  * list per module
* multi module projects
  * assume 1
* per module lang
* per lang compiler, linker 
* vendor deps as modules
* vendor dir 
* customizable include dirs w default

### competition

* http://jmespath.org/
* jq
* interesting features? http://trentm.com/json/
