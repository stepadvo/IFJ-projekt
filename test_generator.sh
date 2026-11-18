#!/bin/bash

expectedOut=("put expected outputs here" "1" "tbd" "tbd" "tbd" "tbd" "iterative: 120
recursive: 120")

for i in {1..6}; do
    printf "\n"
    printf "==========\n"
    printf " TEST %i \n" "$i"
    printf "==========\n"
    printf "Outputted code:\n\n"
    ./test_generator_function.out "$i" > "$i".out && cat "$i".out
    res=$(ic26int "$i".out)
    if [ "$res" == "${expectedOut[$i]}" ]; then
        printf "Returned expected output:\n%s\n" "$res"
    else
        printf "Incorrect output"
        printf "expected:\n%s\n" "${expectedOut[$i]}"
        printf "got:\n%s\n", "$res"
    fi
done

