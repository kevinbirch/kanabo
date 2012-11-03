% KANABO(1) Kanabo Manual
% Kevin Birch <kmb@pobox.com>
% October 23, 2012

# NAME

kanabo - A tool to bludgeon JSON/YAML files from  shell scripts

# SYNOPSIS

kanabo \[*OPTION*\]... --query *expression*  
kanabo \[--shell *shell*\] --file *input-file* --interactive

# DESCRIPTION

Kanabo is a utility to query and examine JSON and YAML files from
shell scripts.

There are two modes of operation: single expression mode and
interactive mode.  In the first, a single JSONPath or YPath query
expression given as a parameter is evaluated and the result is printed
to *stdout*.  In the second, newline separated query expressions will
be read from *stdin* and the result of each printed to *stdout*.

If no *input-file* is specified in single expression mode, then the
input file is read from *stdin*.  The *input-file* is required to be
specified in interactive mode, as *stdin* will be monitored for
expressions to evaluate.

Kanabo - the strong made stronger.

# OPTIONS

-f, \--file *input-file*
:   Specify a file to read the JSON/YAML data from instead of
    *stdin*.  This option is required when using **interactive** mode.

-s, \--shell *shell*
:    Specify the output format for certain structures returned by
     queries (such as associative arrays).  The value of *shell* can
     either **bash** (Bash shell) or **zsh** (Z shell).  The default
     is to use **bash**.   

-v, \--version
:    Print the version information and exit

-w, \--no-warranty
:    Print the no-warranty information and exit

-h, \--help
:    Print the usage summary and exit

# MODE CONTROL

The mode to use is controlled by specifying either of the following
options.  Only one option is allowed.

-Q, \--query *expression*
:    Evaluate a single JSONPath or YPath expression given as *expression*
     and print the result to *stdout*.

-I, \--interactive
:    Enter interactive mode.  Newline separated query expressions will
     be read from *stdin* and the result of each printed to *stdout*.  The
     *input-file* must also be specified.

# INTERACTIVE MODE

details

# EXAMPLES

examples

# NOTES

notes

# CAVEATS

Some properly formatted YPath expressions may not be valid for some
JSON input files due to the stricter nature of JSON.

# SEE ALSO

**jshon**(1)
:    Home page <http://kmkeen.com/jshon>

JSONPath Query Langauge
:    The defacto specification for JSONPath <http://goessner.net/articles/JsonPath>

YPath Query Language
:    The defacto specification for YPath <http://www.pkmurphy.com.au/images/ypathspec.pdf>

GNU Bash shell
:    GNU Bash shell home page <http://www.gnu.org/software/bash>

Z shell
:    Z shell home page <http://zsh.sourceforge.net>

Wikipedia
:    Entry on kanabo <http://en.wikipedia.org/wiki/Kanabo>

# REPORTING BUGS

Please report bugs using the project issue tracker <https://github.com/kevinbirch/kanabo/issues>.

# COPYRIGHT

Copyright (c) 2012 **Kevin Birch**  <kmb@pobox.com>.  All rights reserved.

# COPYING

This program is free software; you can redistribute it and/or modify
it under the terms of an MIT-style License as described in the LICENSE
section.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
LICENSE section for more details.

# LICENSE

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

