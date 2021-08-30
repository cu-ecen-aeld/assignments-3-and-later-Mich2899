#!/bin/sh


filesdir=$1
searchstr=$2

if [ $# -lt 2 ]
then
	echo "Required arguments not provided correctly!"
	exit 1
fi

if [ -d "$filesdir" ]
then
	numfile=$(grep -l "$searchstr" $(find "$filesdir" -type f) | wc -l)
	string=$(grep "$searchstr" $(find "$filesdir" -type f) |wc -l)

	echo "The number of files are $numfile and the number of matching lines are $string !"
else
        echo "directory does not exist!"
        exit 1
fi


