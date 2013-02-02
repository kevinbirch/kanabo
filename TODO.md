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

### evaluator

Enhancements:

* [ ] bad input tests
* [ ] add error codes
* [ ] add error message generator
* [ ] propagate errors everywhere


### parser

Enhancements:

* materialize scalar nodes into json types
* allow negative indices in slices
* ensure memory is freed on secondary failure modes in parser functions
* refactor direct status code setters into function calls
* clean up and extract common patterns
* add more checks for end of input
* wrap output in debug conditionals
  * add exit state function with output
* should we allow an optional qualified path expression after the filter expression in the union expression production?

Features:

* union support
  * allow array indices
  * allow more than two items
* filter support

### loader

Enhancements:

* make loader api return an enum instead of a struct
  * expose input struct, add c'tor/d'tor
  * fold current load functions into single loader from input struct
  * expose methods to make error message from input struct

Features:

* handle anchors and aliases
  * https://en.wikipedia.org/wiki/Hashed_array_tree
* handle tags

### model

Enhancements:

* switch the iterator return types to bool to propigate the evaluation status
* expand scalar nodes to include json subtypes

Features:

* add tree walker api
* add printing visitor
  * for printing scalar node values:
```
    char *format;
    int ret = asprintf(&format, "scalar: '%%.%zds'\n", event->data.scalar.length);
    if(-1 != ret)
    {
        printf(format, event->data.scalar.value);
    }
    free(format);
```

### makefile

Enhancements:

* add version header generation support
* can the warnings from include be supressed somehow? they show up when the depend files don't exist yet

Features:

* add submodule build support
* add package phase impl
* add install phase impl
* extract as its own project

### path languages

* finish jsonpath parser
* extract as project, add as submodule
* ypath support

### unit testing

* upgrade to latest version of check
* try CTest

* CT https://github.com/kr/ct
  * pros: forks test, autogenerates test driver code
  * cons: not a library
* Check http://check.sourceforge.net
  * pro: forks tests, still alive
  * con: installed as a library, macro based test setup
* CTest https://github.com/bvdberg/ctest
  * candidate for improvement using forking
  * pro: embedible - single h file, suite support, setup/teardown support, fixture support
  * con: does not fork, uses macro based test setup
* CuTest http://cutest.sourceforge.net
  * pro: embedible - single c/h file, has an API
  * con: does not fork, not updated in 2 years, many outstanding bugs and patches, originated on Windows
  * non-starter, not maintained
* head-unit https://github.com/boothj5/head-unit
  * pro: has an API, C/C++ support, nice output
  * con: does not fork, installed as a library, no user base?
  * non-starter, no other users
* AceUnit http://aceunit.sourceforge.net
  * pro: JUnit 4 style
  * con: requires Java
  * non-starter, using java is crazy
* CUnit http://cunit.sourceforge.net
  * pro: uses an API, various runners, nice output
  * con: API is very cumbersome, does not fork, not updated for several years, installed as a library, complex, many outstanding bugs and patches, requires glib
  * non-starter, API is terrible

### site

* add site target
* http://tinytree.info

[home]: https://github.com/kevinbirch/kanabo "project home"
[license]: http://www.opensource.org/licenses/ncsa
