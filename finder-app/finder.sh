#!/bin/sh

searchstr=$2
filesdir=$1


if [ $# -lt 2 ]
then 
    echo "Not enough arguments"
    exit 1
fi

if [ -d "$filesdir" ]
then    
    ans1=$(find $filesdir -type f|wc -l)
    ans2=$(grep -r -o -w "$searchstr" $filesdir|wc -l)
    echo "The number of files are ${ans1} and the number of matching lines are ${ans2}"
    exit 0
else    
    echo "no path found"
    exit 1
fi  