#!/usr/bin/env bash

exec 3< <(target/kanabo -f $1 -q '$.store.book.*')
while read -r -u 3 line
do
  declare -A book="$line"
  echo "The book \"${book[title]}\" costs \$${book[price]}"
done
