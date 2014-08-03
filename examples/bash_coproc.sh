#!/usr/bin/env bash

TAX_RATE=0.08875

trap 'yow! trapper keeper' CHLD PIPE

# start kanabo as a named coprocess, in interactive mode loading the file from the 1st argument
coproc kanabo { kanabo $1 ;}

echo "pausing ..."
read
echo "resuming..."

# create an empty array to hold the bookstore menu items
choices=()
# send the query to the coprocess' stdin fd
echo '$.store.book.*' >&${kanabo[1]}
while read -ru ${kanabo[0]} line
do
  echo "line: ${line}"
  # EOD is the sentinal value printed on a line by itself after all other data is printed
  if [ "EOD" == "$line" ]; then
      break
  fi
  declare -A book="$line"
  # for each book, format a menu item for use later by select
  choices+=("\"${book[title]}\" by ${book[author]}: \$${book[price]}")
done

echo
echo "Welcome to our bookstore!"
echo "Please make your selection from these fine books:"
echo
PS3="Your choice: "
select book in "${choices[@]}"
do
    # query for the price of the selected book
    echo "\$.store.book[$((REPLY - 1))].price" >&${kanabo[1]}
    read -ru ${kanabo[0]} price
    echo
    # bash doesn't support floating point arithmetic, so we use bc
    printf "Thank you, with tax your total comes to: \$%.2f\n" $(bc <<< "${price} * ${TAX_RATE} + ${price}")
    break
done

# close the stdin fd for the coprocess, causing it to exit
eval "exec ${kanabo[1]}>&-"
