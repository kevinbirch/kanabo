-*- gfm -*-

# Informational files for [金棒][home] (kanabō)

Copyright (c) 2012 [Kevin Birch](mailto:kmb@pobox.com).  All rights reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of an [MIT-style License][license] as described in
the LICENSE file.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
LICENSE file for more details.

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
* cmbc - http://www.cprover.org/cbmc/
* https://gitorious.org/linted/linted/source/8a9b2c7744af0e2d42419d0fea45c7b37b76930b:
* performance co-pilot? http://oss.sgi.com/projects/pcp/
* add completions for currently loaded model to linenoise
* bug: `--help` option prints error message before help text
* build: libtool integration?
* build: shtool integration?
* make 0.5 release
* switch weather example to yaml config file
* message formatting
  * http://www.tin.org/bin/man.cgi?section=3&topic=snprintf
  * http://stackoverflow.com/questions/69738/c-how-to-get-fprintf-results-as-a-stdstring-w-o-sprintf/69911#69911
* use strlcpy over memcpy for c-strings?
  * http://www.openbsd.org/cgi-bin/cvsweb/src/lib/libc/string/strlcpy.c?rev=1.11;content-type=text%2Fplain
* invert order of static functions?
  * http://www.reddit.com/r/C_Programming/comments/1u1ofw/is_this_code_clike/cednu4t
* interesting competitor: http://jmespath.org/
* various getopt alternatvies: https://news.ycombinator.com/item?id=10687375
* fix license to point to bsd 3 clause
* X_free to dispose_X

### evaluator

* test factory functions
* negative subscript values
* implement union predicates
* implement filter predicates
* support integer and timestamp scalar types
* refactor iteration methods to use filter, tranform, fold
* typeof operator in macros?
  * http://gcc.gnu.org/onlinedocs/gcc/Typeof.html

### parser

* test factory functions
* implement lexer
  * https://github.com/dylan-lang/opendylan/blob/master/sources/dfmc/reader/lexer-transitions.dylan
* implement combinators
* add full tracing
* bug: `$...store` should not be accepted
* bug: '.' should be allowed in a quoted name
* bug: `$` should support predicates
* bug: relational expression should superceed equality expression in filter predicate
* implement escaping
* bug: negative subscripts should return ERR\_EXPECTED\_INTEGER instead of ERR\_UNSUPPORTED\_PRED\_TYPE
* use precondition helpers
* ensure memory is freed on secondary failure modes in parser functions
* refactor direct status code setters into function calls
* add exit state function with output
* negative subscript values
* union support
* filter predicate support
* YAML anchor/alias syntax support
* support integer and timestamp scalar types
* use a more flexibile subscripting syntax
  * ala: http://docs.python.org/2/reference/expressions.html#slicings
  * a predicate is a wildcard, selection or filter
  * a selection is a comma separated list of slice and/or additive expr
  * all literals are copied to the result, all relative paths are evaluated
  * this means you must write @[n] to access unioned subscripts?
  * special case for true subscript predicatess?
  * really this is a generalization of union? they can be additive expr and/or slice
  * can the union item ever be a or expr?
* string functions
* other built-in functions

* use maybe parser in grammar to handle builder errors
* build the jsonpath inline with custom rule parsers
* rename emit package to emitter
* move jsonpath/reader to parser?
* inline jsonpath/model package into jsonpath?
* rename model package to document?
* move nodelist creation to evaluate function evaluator/api.c
* create common maybe.h for enum and others
* use type generic macro instead of vtable
* use sentinels for vararg parsers instead of NULL, error on null args
* create parser error classes with context for building message
* try to reuse parsers instead of creating new every time
* need a balance parser to control bracket balancing
* need to feed stop chars into unqoted name parser
* error on unknown escape sequence
* feed quote char into quoted string parser
* create simplifed static error message reprs for logging
* need tests for all forbidden control chars
* make repr dynamic
* use balanced parser for quoted string
* add character filter delegate to string parser
* use push_back to reset input position to simplify parser message handling

### loader

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
* find undefined behavior code: http://css.csail.mit.edu/stack/
  * clang ubsan
* http://rr-project.org/

### documentation

* man page
  * more jsonpath details
  * interactive mode
  * pandoc?
  * ronn? http://rtomayko.github.io/ronn/ronn-format.7.html
* install file
* readme file
* double check ebnf
* static site
  * http://tinytree.info
  * sphynx?
  * jeckyl?
  * http://staticsitegenerators.net
  * site target
  * http://jashkenas.github.io/docco/
* http://www.reddit.com/r/sideprojectdemo

### build

* http://code.google.com/p/qi-make/
* http://google-engtools.blogspot.fr/2011/08/build-in-cloud-how-build-system-works.html
* http://facebook.github.io/buck/
* http://aosabook.org/en/posa/ninja.html
* http://www.reddit.com/r/programming/comments/204pau/what_are_your_gcc_flags/
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

[home]: https://github.com/kevinbirch/kanabo "project home"
[license]: http://www.opensource.org/licenses/ncsa
