#!/bin/sh

writestr=$2
writefile=$1

if [ $# -lt 2 ]
then 
    echo "Not enough arguments"
    exit 1
fi

echo "$writestr" >"$writefile"
exit 0