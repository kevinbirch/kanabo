-*- gfm -*-

# TODO

## fixes

* parser
  * can `UNEXPECTED_INPUT` error be decomposed into more specific errors? (only used twice)
  * are quoted name tokens shortened by one character?
* move position.h/location.h/input.[c/h] to spacecadet
* evaluator
  * BUG: failed eval of step causes `expression:1:0 evaluator: internal error: model argument is NULL`
  * BUG: provide implicit root for relative paths in top-level expressions
  * track path error location and provide with error type
  * track path of all loaded document nodes, use in error reports
  * add json path to diagnostic
  * track fragment of original query for each added result node
  * update `add_values_to_nodelist_map_iterator` trace with key name
  * update `apply_greedy_wildcard_test` sequence case to trace index and element kind
* switch to structured logging
  * `log(const char *, ...)` vararg params are all subtypes of `event`
  * https://github.com/uber-go/zap
  * https://www.structlog.org/en/stable/getting-started.html
  * https://github.com/google/pblog
  * https://github.com/gward/lolog
  * rework log.h to assume `component_name` is defined before import
  * eliminate all uses of `trace_string`
* jsonpath model dumper secret command line option, nice tree-like layout
  * `-d, --dump <jsonpath>`
  * yaml format?
* update spacecadet with local changes
* emitter
  * return error object with underlying failure
  * `plain` output mode for scalars results sep by newline, collections ignored
* read default output, duplicate settings from env vars
* merge to master
* build 0.8-alpha release

## new features

start: 0.8-alpha, end: 0.9-beta

1. add docker image build job to ci
   * sha1 and version tag for all image builds
   * `latest` and `stable` tags only for (even-numbered?) master builds
   * even number minor versions are stable and odd are experimental?
1. how to select an item of a seq of seq?
   * clean up `evaluate_nodelist` (should be function?)
1. add `scalar()` selector
1. add `parent()` selector
1. join
   * support only paths, not indices
1. defer resolving aliases until document is fully loaded
1. tag selector
1. support full complement of yaml types
   * https://yaml.org/type/index.html
   * new scalar types (timestamp, etc)
   * should unparseable, yet tagged timestamps be marked so they are ignored for comparisons?
1. filter
   * pratt parser for expressions
   * http://effbot.org/zone/simple-top-down-parsing.htm
   * http://www.oilshell.org/blog/2017/03/31.html
1. transformer
   * built-in functions?
1. anchor selector
   * when does it even make sense to use this? only in transformer?
1. pretty output in tty mode, simple output in pipe mode

## documentation

* man page
  * mandoc
    * https://manpages.bsd.lv/mdoc.html 
    * https://linux.die.net/man/7/groff_mdoc
    * https://linux.die.net/man/7/mdoc.samples
  * more jsonpath details
  * interactive mode
  * pandoc?
  * ronn? http://rtomayko.github.io/ronn/ronn-format.7.html
* syntax diagram
  * https://en.wikipedia.org/wiki/Syntax_diagram
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
* example using darksky or weather.gov apis: https://lobste.rs/s/ll18ft/weather_gov_s_free_weather_api

## static analysis

* https://scan.coverity.com/projects/kevinbirch-kanabo/builds/new
* https://lgtm.com/dashboard
* http://fbinfer.com/
* https://github.com/sslab-gatech/qsym
* https://code.google.com/p/american-fuzzy-lop/
* cmbc - http://www.cprover.org/cbmc/
* https://gitorious.org/linted/linted/source/8a9b2c7744af0e2d42419d0fea45c7b37b76930b:
* clang memory sanitizer
  * requires libyaml and check to be built with msan
  * alternative: valgrind
* memory optimizations
  * http://blog.libtorrent.org/2013/12/memory-cache-optimizations/
  * http://www.reddit.com/r/programming/comments/1u660a/the_lost_art_of_c_structure_packing/
  * http://linux.die.net/man/1/pahole
  * https://github.com/arvidn/struct_layout
* function tracing? https://github.com/namhyung/uftrace

## evolution

### global

* libbacktrace instead of execinfo?
  * panic backtrace looks like crap on Linux
* https://gcc.gnu.org/onlinedocs/gcc-6.1.0/gcc/Integer-Overflow-Builtins.html#Integer-Overflow-Builtins

### loader

* track jsonpath step literal for each node
  * add to (simpilifed?) repr dump
* mmap input file and build a no-copy tree that points to strings by byte ranges?
* http://cbor.io/ ?
* parse and reify yaml timestamps
* use sourcelocation instead of position in model, track input file across all nodes
  * reuse the same string object from documentset?
  * write out full position in node repr
* https://github.com/lemire/simdjson ?
* https://dhall-lang.org/# ?

### evaluator

* simple array for small size mapping instances?
* replace memcmp with hash compare?
* store hashcode with entry in bucket chain?
* interning for string module?
  * could greatly reduce memory use for large redundant files
  * diable macro?
  * runtime enable disable?
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

### parser

* print out detail error help with arrow ala clang/gcc
  * use bold style escape seq for whole message line
  * use red style escape seq for `-ERR`

```
>> @.@data.&foo.*bar
-ERR expression:1:3 expected step selector or transformer definition
@.@data.&foo.*bar
  ^----
-ERR expression:1:9 expected step selector or transformer definition
@.@data.&foo.*bar
        ^---
-ERR expression:1:15 expected qualified step definition
@.@data.&foo.*bar
             ^---
```

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
* https://snapcraft.io/ ?
* https://conan.io
* re-enable `-pedantic-errors` flag?
  * can vendored libs be placed in `src/vendor` to have custom build commands?
  * does this have to be done by hand with an existing hook?  can it be detected and automated?

## known limitations

* `-9223372036854775808` (`INT64_MIN`) is not accepted as an index value
* timestamps are not reified internaly

## competition

* http://jmespath.org/
* jq
* interesting features? http://trentm.com/json/
* https://github.com/antonmedv/fx
* tools comparison: https://news.ycombinator.com/item?id=11649142
* https://github.com/johnkerl/miller
