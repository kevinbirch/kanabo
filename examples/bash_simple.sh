#!/usr/bin/env bash

exec 3< <(kanabo -q '$.store.book.*' $1)
while read -r -u 3 line
do
  declare -A book="$line"
  echo "The book \"${book[title]}\" costs \$${book[price]}"
done
