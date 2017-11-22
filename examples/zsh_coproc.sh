#!/usr/bin/env zsh

TAX_RATE=0.08875

# start kanabo as a named coprocess, in interactive mode loading the file from the 1st argument
coproc kanabo -o zsh $1

# create an empty array to hold the bookstore menu items
choices=()
# send the query to the coprocess' stdin fd
print -p '$.store.book.*'
typeset -A book
while read -rp line
do
  # EOD is the sentinal value printed on a line by itself after all other data is printed
  if [[ "EOD" == "$line" ]]; then
      break
  fi
  set -A book ${(Q)${(z)line}}
  # for each book, format a menu item for use later by select
  choices+=("\"${book[title]}\" by ${book[author]}: \$${book[price]}")
done

echo
echo "Welcome to our bookstore!"
echo "Please make your selection from these fine books:"
echo
PROMPT3="Your choice: "
select book in "${choices[@]}"
do
    # query for the price of the selected book
    print -p "\$.store.book[$((REPLY - 1))].price"
    read -rp price
    echo
    printf "Thank you, with tax your total comes to: \$%.2f\n" $((price * TAX_RATE + price))
    break
done

# there doesn't seem to be a way to close a coprocess fd in Zsh
kill -HUP %%
