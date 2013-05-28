#!/usr/bin/env zsh

exec 3< <(kanabo -o zsh -f $1 -q '$.store.book.*')

typeset -A book
while read -r -u 3 line
do
  set -A book ${(Q)${(z)line}}
  echo "The book \"${book[title]}\" costs \$${book[price]}"
done
