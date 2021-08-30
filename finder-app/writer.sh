#!/bin/sh

if [ $# -lt 2 ]
then
        echo "Required arguments not provided correctly!"
        exit 1
fi

writefile=$1
writestr=$2

path=${writefile%\/*}


if [ -d "$path" ]
then 
	#echo "Path already exists! Attempting to write to the file!"
	if [ -e "$writefile" ]
	then
		echo "$writestr" > "$writefile"
	else
                touch "$writefile"
                echo "$writestr" > "$writefile"

	fi

else
	mkdir -p "$path"
	#echo "Path created! Attempting to write to the file!"
	touch "$writefile"
	echo "$writestr" > "$writefile"
fi

if [ -e "$writefile" ]
then
	echo "File successfully written!"
else
	echo "Apologies! File not created!"
fi





