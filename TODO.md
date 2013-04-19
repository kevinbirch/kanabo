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

* don't return -1 from enum return type functions

* fix module api
* connect front & back ends
* cut alpha release tag

### evaluator

#### Enhancements:

* use a evaluator context
* add error message generator

#### Features:

* implement recurisve step
* implement union predicates

### parser

#### Enhancements:

* add human readable names for enum types
* add original expression to jsonpath struct
* slice step must not accept 0
* negative subscripts should return ERR\_EXPECTED\_INTEGER instead of ERR\_UNSUPPORTED\_PRED\_TYPE
* document api with doxygen
* use precondition helpers
* ensure memory is freed on secondary failure modes in parser functions
* refactor direct status code setters into function calls
* clean up and extract common patterns
* add more checks for end of input
* add exit state function with output
* should we allow an optional qualified path expression after the filter expression in the union expression production?

#### Features:

* union support
  * allow array indices
  * allow more than two items
* filter support

### loader

#### Enhancements:

* document api with doxygen
* use precondition helpers
* make loader api return an enum instead of a struct
  * expose input struct, add c'tor/d'tor
  * fold current load functions into single loader from input struct
  * expose methods to make error message from input struct

#### Features:

* handle anchors and aliases
  * https://en.wikipedia.org/wiki/Hashed_array_tree
* handle tags

### model

#### Enhancements:

* add human readable names for enum types
* document api with doxygen
* use precondition helpers
* iterate\_sequence -> sequence\_iterate
* switch the iterator return types to bool to propigate the evaluation status
* expand scalar nodes to include json subtypes
* give nodes a pointer back to their owning document?

#### Features:

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

#### Enhancements:

* add check for check.h header existence

#### Features:

* add version header generation support
* add submodule build support
  * subpaths under $source-root should be preserved, e.g. $src/jsonpath/parser.c -> $target/objects/jsonpath/parser.o
* add package phase impl
* add install phase impl
* add site phase impl
* extract as its own project

### path languages

* finish jsonpath parser
* extract as project, add as submodule
* ypath support

### unit testing

#### Enhancements:

* add command line single test run support
* add more beautiful runner output
* can ctest's style of test macros be used in check?
  * https://github.com/bvdberg/ctest
* eliminate check's end_test macro by creating hidden trampoline function that delegates to user's test function

### site

* add site target
* http://tinytree.info

[home]: https://github.com/kevinbirch/kanabo "project home"
[license]: http://www.opensource.org/licenses/ncsa
