Program: kanabo(1)  
Product: Kanabo 1.0  
Title: Kanabo Manual  
Author: Kevin Birch <kmb@pobox.com>  
Date: 2012-10-23  

## NAME

kanabo - query JSON/YAML files from shell scripts with JSONPath

## SYNOPSIS

`kanabo` [`--format` <format>]  [`--file` <file>] `--query` <expression>  
`kanabo` [`--format` <format>] `--interactive` `--file` <file>

## DESCRIPTION

Kanabo is a utility to bludgeon JSON and YAML files from shell scripts: the 
strong made stronger.  Using [JSONPath][jsonpath], JSON or YAML files can be 
queried and examined just as XPath is used for XML files.

A single JSONPath expression can be evaluated against a document or a series of
expressions can be evaluated interactively.  For the former, a single JSONPath
<expression> is evaluated and the result is printed to *stdout*.  In the later,
newline separated expressions are read from *stdin* and the result of each is
printed to *stdout*.

If no <file> is specified in when evaluating a single <expression>, then the 
<file> is read from *stdin*.  However, the <file> is required to be specified in
second form, as *stdin* will be monitored for expressions to evaluate.

## OPTIONS

These options control the document data source and the output format:

  * `-f`, `--file` <file>
    Specify a file to read the JSON/YAML data from instead of *stdin*.  This
    option is required when using the `--interactive` option.

  * `-o`, `--format` <format>
    Specify the output format for values returned by queries.  The supported
    values of <format> are: **bash** (Bash shell), **zsh** (Z shell), **json**
    or **yaml**.  The default value is **bash**.

These options control expression evaluation.  Only one option is allowed:

  * `-Q`, `--query` <expression>
    Evaluate a single JSONPath <expression> and print the result to *stdout*.

  * `-I`, `--interactive`
    Evaluate expressions interactivly.  Newline separated query expressions will
    be read from *stdin* and the result of each printed to *stdout*.  When using
    this option, the `--file` option is also requred.

Miscellaneous options:

  * `-v`, `--version`
    Print the version information and exit.

  * `-w`, `--no-warranty`
    Print the no-warranty information and exit.

  * `-h`, `--help`
    Print the usage summary and exit.

## OUTPUT FORMATS

The following output formats are supported:

  * [Bash][bash]
  
  * [Zsh][zsh]
  
  * [JSON][json]
  
  * [YAML][yaml]

## INTERACTIVE EVALUATION

details

## JSONPATH

an introduction to jsonpath

## EXAMPLES

Given the following JSON document:

```json
{
  "store":
  {
    "book":
    [ 
      {
        "category": "reference",
        "author": "Nigel Rees",
        "title": "Sayings of the Century",
        "price": 8.95
      },
      {
        "category": "fiction",
        "author": "Evelyn Waugh",
        "title": "Sword of Honour",
        "price": 12.99
      },
      {
        "category": "fiction",
        "author": "Herman Melville",
        "title": "Moby Dick",
        "isbn": "0-553-21311-3",
        "price": 8.99
      },
      { 
        "category": "fiction",
        "author": "J. R. R. Tolkien",
        "title": "The Lord of the Rings",
        "isbn": "0-395-19395-8",
        "price": 22.99
      },
      {
        "category": "fiction",
        "author": "夏目漱石 (NATSUME Sōseki)",
        "title": "吾輩は猫である",
        "isbn": "978-0-8048-3265-6",
        "price": 13.29
      }
    ],
    "bicycle":
    {
      "color": "red",
      "price": 19.95
    }
  }
}
```

This table demonstrates various JSONPath expressions and the results.  N.B. that the above JSON could have also been formatted as the equilvilent YAML document for the exact same results below.

| Expression                    | Result                                         |
| ----------------------------- | ---------------------------------------------- |
| `$.store.book[*].author`      | The authors of all books in the store          |
| `$..author`                   | All authors                                    |
| `$.store.*`                   | All items in the store (5 books and a bicycle) |
| `$.store..price`              | The prices of everything in the store          |
| `$..book[2]`                  | The third book                                 |
| `$..book[-1:]`                | The last book in order                         |
| `$..book[0,1]`, `$..book[:2]`	| The first two books                            |
| `$..book[?(@.isbn)]`          | All books with an isbn number                  |
| `$..book[?(@.price < 10)]`    | All books with a price less than 10            |
| `$..*`                        | All nodes of the JSON document                 |


## JSONPATH EXTENSIONS

The dialect of JSONPath implemented by this program adds some features over those listed in the [defacto specifcation][jsonpath].  The following additional features are supported:

* Predicates can be applied to any step type, not just names (e.g. `$..array[][1]` - the second items of all arrays).
* Node type tests can filter nodes by their type (string, number, boolean, null, array, object).
* Step names can be quoted (e.g. `$.store.'home appliances'.blender`).

## CAVEATS

The dialect of JSONPath implemented by this program drops some features listed in the [defacto specifcation][jsonpath].  The following features defined in the specification are not and never will wbe supported:

* *Bracket* Notation (e.g. `$\['store'\]\['book'\]\[0\]\['title'\]` instead of `$store.book\[0\].title`).
* Script Expressions (e.g. `$..book\[(@.length - 1)\]`)

Bracket expressions provide no demonstrable semantic benefit over the dot notation, and hurts readability.  Script expressions are a very dangerous notion (see [Occupy Babel](http://www.cs.dartmouth.edu/~sergey/langsec/occupy/)), and static compiled languages are not amenable to evaluating expressions of the implementation language at runtime anway.

## SEE ALSO

These references may be of interest to the user of this program:

  * [JSONPath Query Langauge][jsonpath]
  * [GNU Bash shell][bash]
  * [Z shell][zsh]
  * [jshon(1)][jshon]
  * [jq(1)][jq]

## REPORTING BUGS

Please report bugs using the project issue tracker: <https://github.com/kevinbirch/kanabo/issues>.

## COPYRIGHT

Copyright (c) 2012 **Kevin Birch** <kmb@pobox.com>.  All rights reserved.

## COPYING

This program is free software; you can redistribute it and/or modify
it under the terms of an MIT-style License as described in the LICENSE
section.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
LICENSE section for more details.

## LICENSE

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

[jsonpath]: http://goessner.net/articles/JsonPath "The defacto specification"
[bash]: http://www.gnu.org/software/bash
[zsh]: http://zsh.sourceforge.net
[jshon]: http://kmkeen.com/jshon "An alternative tool"
[jq]: http://stedolan.github.com/jq/ "An alternative tool"

