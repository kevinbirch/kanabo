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

* https://www.securecoding.cert.org/confluence/display/seccode/CERT+C+Secure+Coding+Standard
* valgrind
* performance co-pilot? http://oss.sgi.com/projects/pcp/
* add completions for currently loaded model to linenoise
* add safe arithmetic functions
  * http://lists.nongnu.org/archive/html/qemu-devel/2013-01/msg05387.html
  * https://sourceware.org/ml/libc-alpha/2013-12/msg00098.html
* libbacktrace instead of execinfo?
* build: libtool integration?
* build: shtool integration?
* build: http://www.gnu.org/software/make/manual/html_node/Canned-Recipes.html#Canned-Recipes
* fuzzing: https://code.google.com/p/american-fuzzy-lop/
* vagrant boxen for linux, solaris, openbsd

### evaluator

* negative subscript values
* implement union predicates
* implement filter predicates
* support integer and timestamp scalar types
* refactor iteration methods to use filter, tranform, fold
* jit? http://eli.thegreenplace.net/2013/10/17/getting-started-with-libjit-part-1
  * https://pauladamsmith.com/blog/2015/01/how-to-get-started-with-llvm-c-api.html
* streaming mode
  * https://github.com/fizx/sit
  * http://stackoverflow.com/questions/13083491/looking-for-big-sample-dummy-json-data-file
  * https://github.com/udp/json-parser
  * https://github.com/zeMirco/sf-city-lots-json
  * https://github.com/seductiveapps/largeJSON
* interesting features? http://trentm.com/json/

### parser

* can we mmap the input file and build a no-copy tree that points to strings by byte ranges?
* implement lexer
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
  * allow array indices
  * allow more than two items
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

### loader

* support sets and ordered maps

### unit testing

* implement parameterized tests (some expression given to BEGIN_TEST that is inserted in the test boiler plate)
* add more beautiful runner output
* can ctest's style of test macros be used in check?
  * https://github.com/bvdberg/ctest
* eliminate check's end_test macro by creating hidden trampoline function that delegates to user's test function
* fuzz testing: https://en.wikipedia.org/wiki/API_Sanity_Checker
* find undefined behavior code: http://css.csail.mit.edu/stack/
* http://sourceforge.net/p/check/code/HEAD/tree/trunk/src/check_impl.h ?
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

### build

* pkg-config - http://www.freedesktop.org/wiki/Software/pkg-config/
* http://code.google.com/p/qi-make/
* http://google-engtools.blogspot.fr/2011/08/build-in-cloud-how-build-system-works.html
* http://facebook.github.io/buck/
* http://aosabook.org/en/posa/ninja.html
* https://github.com/mkpankov/qake
* gnu make 4.0
  * try --trace to check out rule ordering
* define, eval to create dynamic rules and shell?
* guix? http://dthompson.us/reproducible-development-environments-with-gnu-guix.html

[home]: https://github.com/kevinbirch/kanabo "project home"
[license]: http://www.opensource.org/licenses/ncsa
