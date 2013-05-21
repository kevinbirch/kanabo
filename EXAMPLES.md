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

## EXAMPLES

The examples below use the following JSON document as their input data:

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

Using this document, we want to produce the output:

```shell
The book "Sayings of the Century" costs $8.95
The book "Sword of Honour" costs $12.99
The book "Moby Dick" costs $8.99
The book "The Lord of the Rings" costs $22.99
The book "吾輩は猫である" costs $13.29
```

This can be best accompilshed as a one-shot query using the `--query` argument with the JSONPath `$.store.book.*`.  The wildcard is greedy, so it will expand the sequence value of the `book` key into a list of book mappings.  Using the Bash or Zsh output formats will write each result item of the JSONPath query on a separate line for easy parsing by the shell.

### Bash

To produce the above output using the Bash shell and the Bash output format (which is the default), use this script:

```bash
exec 3< <(kanabo -f $1 -q '$.store.book.*')
while read -r -u 3 line
do
  declare -A book="$line"
  echo "The book \"${book[title]}\" costs \$${book[price]}"
done
```

The first line opens a new file descriptor (number 3) for reading, and connects the standard output of the kanabo process to it.  The path to the input file is an argument to this script itself.  Next. a while loop calls `read` to read lines from file descriptor 3 and store the text in the variable `line`.  The body of the loop then creates an associative array named `book` with the contents of the line.  The Bash output format emits each mapping result in the associative array subscript syntax.  Mapping values that have spaces will be quoted with single quotes.  Automatic expansion by Bash will take care of word splitting and de-quoting for us.

An example of the Bash output format would be:

```bash
$ kanabo -q '$.store.book[1]' -o bash < path/to/input.json
([category]=fiction [author]='Evelyn Waugh' [title]='Sword of Honour' [price]=12.99 )
```

### Zsh

To produce the above output using the Zsh shell and the Zsh output format, use this script:

```zsh
exec 3< <(kanabo -o zsh -f $1 -q '$.store.book.*')
typeset -A book
while read -r -u 3 line
do
  set -A book ${(Q)${(z)line}}
  echo "The book \"${book[title]}\" costs \$${book[price]}"
done
```

The first line opens a new file descriptor (number 3) for reading, and connects the standard output of the kanabo process to it.  The path to the input file is an argument to this script itself.  Next an associtive array named `book` is declared.  In a while loop `read` is called to read lines from file descriptor 3 and store the text in the variable `line`.  The body of the loop sets the content of `book` to the word split and de-quoted values in `line`.  The Zsh output formats each mapping result from the JSONPath query as a list of key value pairs separated by spaces.  Word expansion is not automatic in Zsh, so each line must be word split according to the standard rules using the `z` parameter expansion flag.  Keys and values that have spaces in them are quoted with single quotes, and need to be un-quoted using the `Q` parameter expansion flag.

An example of the Zsh output format would be:

```zsh
$ kanabo -q '$.store.book[1]'-o zsh < path/to/input.json
category fiction author 'Evelyn Waugh' title 'Sword of Honour' price 12.99
```

[home]: https://github.com/kevinbirch/kanabo "project home"
[wiki]: http://en.wikipedia.org/wiki/Kanabō "Wikipedia entry for kanabō"
[mit]: http://www.opensource.org/licenses/ncsa
[license]: https://github.com/kevinbirch/kanabo/blob/master/LICENSE.md "license file"
