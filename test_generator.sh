#!/bin/bash

expectedOut=("put expected outputs here" "1" "[2][4][5]
result: 17" "safe: true
either: true
ready" "sign: 1
sum: 11
descending: 6 4 2
exclusive: 0 1 2" "iterative: 120
recursive: 120" "values: true () 1.50
unit_print: ()
res: 7.50")
message=("put test message here" "Simple test" "Test from funexp extension example" "Test from Boolthen extension example" "Test from Control extension example" "Test from factorial example" "Test yet untested")
passed="true"
failedArr=""

for i in {1..6}; do
    printf "\n"
    printf "==========\n"
    printf " TEST %i \n" "$i"
    printf "%s\n" "${message[$i]}"
    printf "==========\n"
    printf "Outputted code:\n\n"
    ./test_generator_function.out "$i" > "$i".out
    cat "$i".out
    res=$(ic26int "$i".out)
    if [ "$res" == "${expectedOut[$i]}" ]; then
        printf "Returned expected output:\n%s\n" "$res"
    else
        passed="false"
        failedArr=$(printf "%s%s" "$failedArr" "$i")
        printf "Incorrect output"
        printf "expected:\n%s\n" "${expectedOut[$i]}"
        printf "got:\n%s\n" "$res"
    fi
done

if [ "$passed" == "true" ]; then
    printf "\n"
    printf "\n"
    printf "==================\n"
    printf " ALL TESTS PASSED \n"
    printf "==================\n"
else
    printf "\n"
    printf "\n"
    printf "====================\n"
    printf " THESE TESTS FAILED \n"
    printf " %s \n" "$failedArr"
    printf "====================\n"
fi

