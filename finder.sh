#!/bin/bash

argc=$#                             # Store the number of arguments (TLDs) given
files=$(ls out)                     # Store all the output files

for argc do                         # Iterate arguments (TLDs) - for every TLD do
    tld=".$argc "                   # Note the dot '.' and whitespace ' '
    count=0                         # Reset counter
    for file in $files; do          # Iterate output files - for every output file do
        match=$(grep "$tld" "out/$file")    # Store matching lines
        for curr in $match; do      # Iterate match - for every element in match
            if [[ $curr =~ ^[0-9]+$ ]] ; then # Check if element is a number
                let count+=$curr    # Increase by num_of_appearances
            fi
        done
    done
    printf "Top Level Domain (TLD) %s was found %d times\n" $argc $count
done