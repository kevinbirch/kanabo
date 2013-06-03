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

* security
  * https://www.securecoding.cert.org/confluence/display/seccode/CERT+C+Secure+Coding+Standard
  * https://www.securecoding.cert.org/confluence/display/seccode/MEM07-C.+Ensure+that+the+arguments+to+calloc%28%29%2C+when+multiplied%2C+do+not+wrap
  * https://www.securecoding.cert.org/confluence/display/seccode/MEM08-C.+Use+realloc%28%29+only+to+resize+dynamically+allocated+arrays
  * https://www.securecoding.cert.org/confluence/display/seccode/MEM09-C.+Do+not+assume+memory+allocation+functions+initialize+memory
* add completions for currently loaded model to linenoise

### evaluator

#### Enhancements:

* document api with doxygen

#### Features:

* negative subscript values
* implement union predicates
* implement filter predicates

### parser

* bug: `$...store` should not be accepted
* bug: '.' should be allowed in a quoted name
* bug: `$` should support predicates
* bug: relational expression should superceed equality expression in filter predicate

#### Enhancements:

* implement lexer
* implement combinators
* add full tracing
* add human readable names for enum types
* negative subscripts should return ERR\_EXPECTED\_INTEGER instead of ERR\_UNSUPPORTED\_PRED\_TYPE
* document api with doxygen
* use precondition helpers
* ensure memory is freed on secondary failure modes in parser functions
* refactor direct status code setters into function calls
* add exit state function with output

#### Features:

* negative subscript values
* union support
  * allow array indices
  * allow more than two items
* filter predicate support
* YAML anchor/alias syntax support?

### loader

* bug: check for yaml type tags: http://yaml.org/type/index.html

#### Enhancements:

* support tags
* support integer and timestamp scalar types
* document api with doxygen

#### Features:

* handle anchors and aliases
  * https://en.wikipedia.org/wiki/Hashed_array_tree
  * use for mappings as well
* handle tags

### model

#### Enhancements:

* replace x\_get\_y with x\_y
* add human readable names for enum types
* document api with doxygen
* use precondition helpers
* iterate\_sequence -> sequence\_iterate
* switch the iterator return types to bool to propigate the evaluation status
* expand scalar nodes to include json subtypes
* give nodes a pointer back to their owning document?
* support iteger and timestamp scalar types

### unit testing

#### Enhancements:

* implement parameterized tests (some expression given to BEGIN_TEST that is inserted in the test boiler plate)
* add more beautiful runner output
* can ctest's style of test macros be used in check?
  * https://github.com/bvdberg/ctest
* eliminate check's end_test macro by creating hidden trampoline function that delegates to user's test function

### documentation

* man page
  * more jsonpath details
  * interactive mode
* install file
* readme file
* examples
* double check ebnf
* static site
  * http://tinytree.info
  * site target

[home]: https://github.com/kevinbirch/kanabo "project home"
[license]: http://www.opensource.org/licenses/ncsa
