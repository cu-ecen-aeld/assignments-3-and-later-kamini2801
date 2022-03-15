#!/bin/sh

writestr=$2
writefile=$1

if [ $# -lt 2 ]
then 
    echo "Not enough arguments"
    exit 1
fi

echo "$writestr" >"$writefile"
if [ ! -f $writefile ]
then
    echo "File not created"
    exit 1
fi
exit 0