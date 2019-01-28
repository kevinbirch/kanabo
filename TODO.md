-*- gfm -*-

# TODO

## fixes

* circle ci build
* code coverage
* bug printing path query results causes next command parse to crash
* loader
  * scalars
    * concrete subtypes, each holding reified value
    * fully parse all scalar types
  * use `String` for anchors and tag name
  * failure in `add_node` or error from libyaml are fatal
  * add extra context string to capture scalar value *or* libyaml message
* parser
  * don't copy lexeme, don't paste minus and integer lexemes together
  * eliminate possible error in lexeme extraction (then we don't need the internal error?)
  * don't use callback between scanner and parser for errors, track error vector in each
    * use single add_parser_error function
    * why are postion macros different for parser and scanner?
* evaluator
  * track path of all loaded document nodes, use in error reports
  * add json path to diagnostic
  * track fragment of original query for each added result node
  * update `add_values_to_nodelist_map_iterator` trace with key name
  * update `apply_greedy_wildcard_test` sequence case to trace index and element kind
  * concrete predicate subtypes
* switch to structured logging
  * `log(const char *, ...)` vararg params are all subtypes of `event`
  * https://github.com/uber-go/zap
  * https://www.structlog.org/en/stable/getting-started.html
  * rework log.h to assume `component_name` is defined before import
  * eliminate all uses of `trace_string`
* jsonpath model dumper secret command line option, nice tree-like layout
  * `-d, --dump <jsonpath>`
* switch weather example to yaml config file
* memory leaks
  * (should be fixed) ignored keys and values are leaked
  * smoke test on linux w/ leak check enabled
* update spacecadet
  * move changes back
  * add maybe, xcalloc, others?

## new features

start: 0.8-alpha, end: 0.9-beta

1. how to select an item of a seq of seq?
1. add `scalar()` selector
1. add `parent()` selector
1. join
   * support only paths, not indices
1. tag selector
1. new scalar types (timestamp, etc)
1. filter
   * pratt parser for expressions
   * http://effbot.org/zone/simple-top-down-parsing.htm
   * http://www.oilshell.org/blog/2017/03/31.html
1. transformer
   * built-in functions?
1. anchor selector
   * when does it even make sense to use this? only in transformer?
1. `set` command to save variables
1. response ouput: `+OK <line count>` `-ERR <code> <location> "explanation"`
1. setting for end-of-ouput sentinel? use `+OK`/`-ERR` instead?
1. pretty output in tty mode, simple output in pipe mode

## documentation

* man page
  * mandoc - https://manpages.bsd.lv/mdoc.html
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

## static analysis

* valgrind
* memory optimizations
  * http://blog.libtorrent.org/2013/12/memory-cache-optimizations/
  * http://www.reddit.com/r/programming/comments/1u660a/the_lost_art_of_c_structure_packing/
  * http://linux.die.net/man/1/pahole
  * https://github.com/arvidn/struct_layout
* http://fbinfer.com/
* https://github.com/sslab-gatech/qsym
* https://code.google.com/p/american-fuzzy-lop/
* cmbc - http://www.cprover.org/cbmc/
* https://gitorious.org/linted/linted/source/8a9b2c7744af0e2d42419d0fea45c7b37b76930b:
* performance co-pilot? http://oss.sgi.com/projects/pcp/
* fuzz testing: https://en.wikipedia.org/wiki/API_Sanity_Checker
* find undefined behavior code
  * http://css.csail.mit.edu/stack/
* function tracing? https://github.com/namhyung/uftrace

## evolution

### global

* libbacktrace instead of execinfo?
  * panic backtrace looks like crap on Linux

### parser

* try to reuse parsers instead of creating new every time

### loader

* can we mmap the input file and build a no-copy tree that points to strings by byte ranges?
* support sets and ordered maps
* http://cbor.io/ ?

### evaluator

* simple array for small size mapping instances?
* replace memcmp with hash compare?
* store hashcode with entry in bucket chain?
* interning for string module?
  * diable macro
  * runtime enable disable
* computed goto dispatch table?
  * http://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables
* safe arithmetic functions?
  * http://lists.nongnu.org/archive/html/qemu-devel/2013-01/msg05387.html
  * https://sourceware.org/ml/libc-alpha/2013-12/msg00098.html
* jit compiler?
  * http://eli.thegreenplace.net/2013/10/17/getting-started-with-libjit-part-1/
  * http://www.stephendiehl.com/llvm/
* streaming mode?
  * https://github.com/fizx/sit
  * http://stackoverflow.com/questions/13083491/looking-for-big-sample-dummy-json-data-file
  * https://github.com/udp/json-parser
  * https://github.com/zeMirco/sf-city-lots-json
  * https://github.com/seductiveapps/largeJSON
* support in-place document patching?
  * should there be a stack of edits atop of loaded tree (immutable loads?)
  * can whole documents be saved and named? (`${doc-name or index}` `$index`?)
* use `flatten` attribute on evaluator, parser core functions?
  * https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#Common-Function-Attributes
* add completions for currently loaded model to linenoise

### emitters

* check return status value of all calls to output funtions?

### unit testing

* http://code.google.com/p/cmockery/ ?
* implement parameterized tests (some expression given to BEGIN_TEST that is inserted in the test boiler plate)
* add more beautiful runner output
* can ctest's style of test macros be used in check?
  * https://github.com/bvdberg/ctest
* eliminate check's end_test macro by creating hidden trampoline function that delegates to user's test function
  * https://mort.coffee/home/obscure-c-features/
* http://rr-project.org/
* https://github.com/nivox/quickcheck4c
* https://github.com/silentbicycle/theft
* run diff-based tests from shell script
  * https://github.com/tavianator/bfs/blob/master/tests.sh

### build

* pkg-config - http://www.freedesktop.org/wiki/Software/pkg-config/
* compilation database?
  * use guile?
  * all files stored into target, make clean to reset
  * post compile step 1 dedupes compilation log with raw compilation db
  * post compile step 2 converts raw compilation db to json
  * pre/post-compile/build hooks?

## competition

* http://jmespath.org/
* jq
* interesting features? http://trentm.com/json/
* https://github.com/antonmedv/fx
* tools comparison: https://news.ycombinator.com/item?id=11649142
