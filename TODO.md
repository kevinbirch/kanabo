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
* add completions for currently loaded model to linenoise
* bug: `--help` option prints error message before help text
* build: libtool integration?
* build: shtool integration?

### evaluator

* negative subscript values
* implement union predicates
* implement filter predicates
* support integer and timestamp scalar types
* refactor iteration methods to use filter, tranform, fold

### parser

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

* forbid non-scalars as mapping keys
* hashtable for mappings
* handle anchors and aliases
* support sets and ordered maps
* allow empty scalar values, forbid empty scalar keys

### unit testing

* implement parameterized tests (some expression given to BEGIN_TEST that is inserted in the test boiler plate)
* add more beautiful runner output
* can ctest's style of test macros be used in check?
  * https://github.com/bvdberg/ctest
* eliminate check's end_test macro by creating hidden trampoline function that delegates to user's test function
* fuzz testing: https://en.wikipedia.org/wiki/API_Sanity_Checker
* find undefined behavior code: http://css.csail.mit.edu/stack/

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

### build

* http://code.google.com/p/qi-make/
* http://google-engtools.blogspot.fr/2011/08/build-in-cloud-how-build-system-works.html
* http://facebook.github.io/buck/
* http://aosabook.org/en/posa/ninja.html
* gnu make 4.0
  * try --trace to check out rule ordering
* define, eval to create dynamic rules and shell?

[home]: https://github.com/kevinbirch/kanabo "project home"
[license]: http://www.opensource.org/licenses/ncsa
