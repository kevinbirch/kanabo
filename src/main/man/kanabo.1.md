Program: kanabo(1)  
Product: Kanabo 1.0  
Title: Kanabo Manual  
Author: Kevin Birch <kmb@pobox.com>  
Date: 2012-10-23  

## NAME

kanabo - query JSON/YAML files from shell scripts with JSONPath

## SYNOPSIS

`kanabo` \[`-o` \<format\>\] \[`-d` \<strategy\>\] `-q` \<jsonpath\> \[\<file\> | '-'\]  
`kanabo` \[`-o` \<format\>\] \[`-d` \<strategy\>\] \[\<file\>\]

## DESCRIPTION

Kanabo is a utility to bludgeon JSON and YAML files from shell scripts: the
strong made stronger.  Using [JSONPath][jsonpath], JSON or YAML files can be
queried and examined just as XPath is used for XML files.

A single JSONPath expression can be evaluated against a document or a series of
expressions can be evaluated interactively.  For the former, a single JSONPath
\<expression\> is evaluated and the result is printed to *stdout*.  In the later,
newline separated expressions are read from *stdin* and the result of each is
printed to *stdout*.  In the first form `-' can be used as an alias for *stdin*.

If no \<file\> is specified in when evaluating a single \<expression\>, then the
\<file\> is read from *stdin*.  Since *stdin* cannot be used to read in the second
form, the \<file\> should be specified on the command line or using the `:load'
command.

## OPTIONS

  * `-o`, `--output` \<format\>
    Specify the output format for values returned by queries.  The supported
    values of \<format\> are: **bash** (Bash shell), **zsh** (Z shell), **json**
    or **yaml**.  The default value is **bash**.

  * `-q`, `--query` \<expression\>
    Evaluate a single JSONPath \<expression\>, print the result to *stdout* and exit.

  * `-d`, `--duplicate` \<stratety\>
    Specify how to handle duplicate mapping keys.  The supported values of \<strategy\>
    are: **clobber** (replace duplicates), **warn** (replace duplicates and print a
    warning message) or **fail** (quit the program).  The default value is **clobber**.

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
    Each query result will be printed on a separate line, with object
    key value pairs and sequences printed as associative arrays and 
    indexed arrays respectively.  Any string values with embedded whitespace 
    will be wrapped with single quotes.  Object key values are already
    delimited by square brackets so they will not be quoted.

    ```bash
    $ kanabo --query '$.store.book.*' --output bash < inventory.json
    ([category]=reference [author]='Nigel Rees' [title]='Sayings of the Century' [price]=8.95 )
    ([category]=fiction [author]='Evelyn Waugh' [title]='Sword of Honour' [price]=12.99 )
    ([category]=fiction [author]='Herman Melville' [title]='Moby Dick' [isbn]=0-553-21311-3 [price]=8.99 )
    ([category]=fiction [author]='J. R. R. Tolkien' [title]='The Lord of the Rings' [isbn]=0-395-19395-8 [price]=22.99 )
    ([category]=fiction [author]='夏目漱石 (NATSUME Sōseki)' [title]=吾輩は猫である [isbn]=978-0-8048-3265-6 [price]=13.29 )
    ```

  * [Zsh][zsh]  
    Each query result will be printed on a separate line, with object
    key value pairs and sequences both printed as space separated lists.  Any 
    string values with embedded whitespace will be wrapped with single quotes.
  
    ```sh
    $ kanabo --query '$.store.book.*' --output zsh < inventory.json
    category reference author 'Nigel Rees' title 'Sayings of the Century' price 8.95
    category fiction author 'Evelyn Waugh' title 'Sword of Honour' price 12.99
    category fiction author 'Herman Melville' title 'Moby Dick' isbn 0-553-21311-3 price 8.99
    category fiction author 'J. R. R. Tolkien' title 'The Lord of the Rings' isbn 0-395-19395-8 price 22.99
    category fiction author '夏目漱石 (NATSUME Sōseki)' title 吾輩は猫である isbn 978-0-8048-3265-6 price 13.29
    ```

  * [JSON][json]  
    The result of the query will be printed as unformatted JSON.

    ```sh
    $ kanabo --query '$.store.book.*' --output json < inventory.yaml
    [{"category":"reference","author":"Nigel Rees","title":"Sayings of the Century","price":8.95},{"category":"fiction","author":"Evelyn Waugh","title":"Sword of Honour","price":12.99},{"category":"fiction","author":"Herman Melville","title":"Moby Dick","isbn":"0-553-21311-3","price":8.99},{"category":"fiction","author":"J. R. R. Tolkien","title":"The Lord of the Rings","isbn":"0-395-19395-8","price":22.99},{"category":"fiction","author":"夏目漱石 (NATSUME Sōseki)","title":"吾輩は猫である","isbn":"978-0-8048-3265-6","price":13.29}]
    ```

  * [YAML][yaml]  
    The result of the query will be printed as formatted YAML.

    ```sh
    $ kanabo --query '$.store.book.*' --output yaml < inventory.yaml
    %YAML 1.1
    ---
    - category: "reference"
      author: "Nigel Rees"
      title: "Sayings of the Century"
      price: 8.95
    - category: "fiction"
      author: "Evelyn Waugh"
      title: "Sword of Honour"
      price: 12.99
    - category: "fiction"
      author: "Herman Melville"
      title: "Moby Dick"
      isbn: "0-553-21311-3"
      price: 8.99
    - category: "fiction"
      author: "J. R. R. Tolkien"
      title: "The Lord of the Rings"
      isbn: "0-395-19395-8"
      price: 22.99
    - category: "fiction"
      author: "夏目漱石 (NATSUME Sōseki)"
      title: "吾輩は猫である"
      isbn: "978-0-8048-3265-6"
      price: 13.29
      ```

## INTERACTIVE EVALUATION

xxx - details

## JSONPATH

A JSONPath expression is composed of a series of steps beginning with a `$` and
separated by a `.`. The result of evaluating an expression is a list of nodes
from the original document.

xxx - details

### Examples

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

This table demonstrates various JSONPath expressions and the results.  N.B. that
the above JSON could have also been formatted as the equivalent YAML document
for the exact same results below.

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
| `$..*`                        | All nodes of the document                      |

## CAVEATS

The dialect of JSONPath implemented by this program differs in some ways from
the [de facto specification][jsonpath]:

  * Predicates can be applied to any step type, not just name tests (e.g. 
    `$..array()[1]` - the second item of all array elements in the document).
  * Node type tests can filter nodes by their type (string, number, boolean,
    null, array, object).
  * Step names can be quoted (e.g. `$.store.'home.appliances'.blender` to escape
    the embedded `.`).
  * Bracket notation (e.g. `$['store']['book'][0]['title']` instead of
    `$store.book[0].title`) is not supported.  Bracket notation provides no
    semantic benefit over the dot notation and hurts readability.
  * Script Expressions (e.g. `$..book[(@.length - 1)]`) are not supported.
    Script expressions are a very dangerous notion (see 
    [Occupy Babel](http://www.cs.dartmouth.edu/~sergey/langsec/occupy/)).  Use
    filter expressions instead.

## SEE ALSO

These references may be of interest to the user of this program:

  * [JSONPath Query Language][jsonpath]
  * [GNU Bash shell][bash]
  * [Z shell][zsh]
  
These alternatives to Kanabo may be useful to comapre features:

  * [jshon](http://kmkeen.com/jshon)
  * [jq](http://stedolan.github.com/jq/)
  * [JSON.sh](https://github.com/dominictarr/JSON.sh)
  * [json-bash](https://github.com/ingydotnet/json-bash)
  * [jsawk](https://github.com/micha/jsawk)
  * [jsonpipe](https://github.com/zacharyvoase/jsonpipe)
  * [jsontool](http://trentm.com/json/)
  * [ticktick](https://github.com/kristopolous/TickTick)

## REPORTING BUGS

Please report bugs using the project issue tracker: *https://github.com/kevinbirch/kanabo/issues*.

## COPYRIGHT

Copyright (c) 2012-2019 **Kevin Birch** <kmb@pobox.com>.  All rights reserved.

## COPYING

This program is free software; you can redistribute it and/or modify
it under the terms described in the LICENSE section.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
LICENSE section for more details.

## LICENSE

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal with
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

- Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimers.
- Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimers in the documentation and/or
  other materials provided with the distribution.
- Neither the names of the copyright holders, nor the names of the authors, nor
  the names of other contributors may be used to endorse or promote products
  derived from this Software without specific prior written permission.

**THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE CONTRIBUTORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.**

[jsonpath]: http://goessner.net/articles/JsonPath "The de facto specification"
[bash]: http://www.gnu.org/software/bash
[zsh]: http://zsh.sourceforge.net
[json]: http://json.org
[yaml]: http://yaml.org
