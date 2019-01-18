-*- gfm -*-

# TODO

## fixes

* clean up logging in emitter module
* remove all redundant casts to `(Node *)`
* ignored keys and values are leaked
* x_free to dispose_x
* panic on illegal downcast
* \_POSIX_C_SOURCE usage?
* `dispose_node` should be type-generic macro
* `node` upcast should be a type-generic macro
* mapping
  * fnv1 hash?
  * use simple array for small collections?
* normalize uses of `%zd` to `%lld` where necessary
* preserve input file names to use with warning lines (`loader/yaml.c:add_to_mapping`)
  * print dupe key name when scalar no
* normalize all panic messages: `<module>: <operation>: <failure>`
* formatting for `String`
  * http://www.tin.org/bin/man.cgi?section=3&topic=snprintf
  * http://stackoverflow.com/questions/69738/c-how-to-get-fprintf-results-as-a-stdstring-w-o-sprintf/69911#69911
* switch to structured logging
  * `log(const char *, ...)` vararg params are all subtypes of `event`
  * https://github.com/uber-go/zap
  * https://www.structlog.org/en/stable/getting-started.html
  * rework log.h to assume `component_name` is defined before import
  * eliminate all uses of `trace_string`
* don't use callback between scanner and parser for errors, track error vector in each
  * use single add_parser_error function
  * why are postion macros different for parser and scanner?
* clean up `xxx` notes
* libbacktrace instead of execinfo?
  * panic backtrace looks like crap on Linux?
* loader error handing
  * failure in `add_node` or error from libyaml are fatal
  * add extra context string to capture scalar value *or* libyaml message
* model dumper w/ secret command line option, nice tree-like layout
  * `--output=ast`
* change license to bsd 3 clause
* add safe arithmetic functions
  * http://lists.nongnu.org/archive/html/qemu-devel/2013-01/msg05387.html
  * https://sourceware.org/ml/libc-alpha/2013-12/msg00098.html
* switch weather example to yaml config file
* various getopt alternatvies: https://news.ycombinator.com/item?id=10687375

* update spacecadet
  * move changes back
  * add maybe, xcalloc, others?
  * print stack trace on xcalloc failure
  * mv spacecadet,linenose,yaml,check stuff to vendor

## new features

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
1. `set` command to save variables
1. setting for end-of-ouput sentinel
1. pretty output in tty mode, simple output in pipe mode
1. response ouput: `+OK <line count>` `-ERR <code> <location> "explanation"`
1. add completions for currently loaded model to linenoise

## loader

* can we mmap the input file and build a no-copy tree that points to strings by byte ranges?
* regex's can use '{}' repetition counts
* support sets and ordered maps
* http://cbor.io/ ?
* track path of all loaded document nodes, use in error reports

## parser

* fix maybe usage
* must_make_regex (uses statement expr)
* parse integers w/o strtoll
  * don't copy lexeme, don't paste minus and integer lexemes together
  * https://github.com/gcc-mirror/gcc/blob/master/libiberty/strtoll.c
  * https://sourceware.org/git/?p=glibc.git;a=blob;f=stdlib/strtol_l.c;h=28ea4bced19cae66440901257d3681985fec220b;hb=HEAD
* eliminate possible error in lexeme extraction
  * then we don't need the internal error?

## evaluator

* add json path to diagnostic
* try to reuse parsers instead of creating new every time
* track fragment of original query for each added result node
* update evaluator err msgs to specify which are internal errors
* update `add_values_to_nodelist_map_iterator` trace with key name
* update `apply_greedy_wildcard_test` sequence case to trace index and element kind
* concrete predicate subtypes
* replace memcmp with hash compare?
* store hashcode with entry in bucket chain?
* interning for string module
  * diable macro
  * runtime enable disable
* computed goto dispatch table?
  * http://eli.thegreenplace.net/2012/07/12/computed-goto-for-efficient-dispatch-tables
* refactor iteration methods to use filter, tranform, fold

## unit testing

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

## build

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
* make secondary expansion?
  * https://www.gnu.org/software/make/manual/html_node/Secondary-Expansion.html
* try fbinfer: http://fbinfer.com/
* try qsym: https://github.com/sslab-gatech/qsym
* enable warning for redundant warning flags
* build security
  * flags all builds: `-Werror=format-security -fstack-protector`
  * flags for debug: `-D_FORTIFY_SOURCE=1 -O1`
  * flags for release: `-D_FORTIFY_SOURCE=2 -pie -fPIE -O2 -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack`
  * https://blog.erratasec.com/2018/12/notes-on-build-hardening.html
    If you are building code using gcc on Linux, here are the options/flags you should use:
    `-Wall -Wformat -Wformat-security -Werror=format-security -fstack-protector -pie -fPIE -D_FORTIFY_SOURCE=2 -O2 -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack`
    If you are more paranoid, these options would be:
    `-Wall -Wformat -Wformat-security -Wstack-protector -Werror -pedantic -fstack-protector-all --param ssp-buffer-size=1 -pie -fPIE -D_FORTIFY_SOURCE=2 -O1 -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack`
  * https://blog.quarkslab.com/clang-hardening-cheat-sheet.html

## static analysis

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
* fuzz testing: https://en.wikipedia.org/wiki/API_Sanity_Checker
* find undefined behavior code
  * http://css.csail.mit.edu/stack/
  * clang ubsan

## evolution

* jit compiler for evaluator?
  * http://eli.thegreenplace.net/2013/10/17/getting-started-with-libjit-part-1/
  * http://www.stephendiehl.com/llvm/
* streaming mode
  * https://github.com/fizx/sit
  * http://stackoverflow.com/questions/13083491/looking-for-big-sample-dummy-json-data-file
  * https://github.com/udp/json-parser
  * https://github.com/zeMirco/sf-city-lots-json
  * https://github.com/seductiveapps/largeJSON
* function tracing? https://github.com/namhyung/uftrace
* support in-place document patching
  * should there be a stack of edits atop of loaded tree (immutable loads?)
  * can whole documents be saved and named? (`${doc-name or index}` `$index`?)

## competition

* http://jmespath.org/
* jq
* interesting features? http://trentm.com/json/
* https://github.com/antonmedv/fx
* tools comparison: https://news.ycombinator.com/item?id=11649142
