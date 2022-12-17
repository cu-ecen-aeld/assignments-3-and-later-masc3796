#!/bin/bash

# Updated 12/10/2022
# $1 = Path to a Directory 
# $2 = Search String 
# Ouputs = String containing number of files and lines in file with said string

#Exit with error 1 if either argument is unspecified
if [[ $# -lt 2 ]]
then 
    echo "Missing Argument(s), Exiting..."
    exit 1
fi 

#Exit with error 1 if 1st argument is not a directory
if [ ! -d $1 ] 
then 
    echo "Invalid Directory, Exiting..."
    exit 1
fi

#Recursively Grep for the pattern and exclude results where the pattern isn't matched

filecount=0
linecount=0
regex='\:([0-9])+'
for line in $(grep -cR $2 $1 | grep -v :0);
    #Note, lesson learned! piping text into loop causes it to run in a subshell
    #Hence it cannot update globals like $linecount in a 

    do
    #echo $line
    
    #Could we count these lines without a regex match? 
    if [[ $line =~ $regex ]]
    then 
        match="${BASH_REMATCH[1]}"
        linecount=$((linecount + match))
    fi

    filecount=$((filecount+1))
    #echo $filecount
    #echo $linecount 
done 

#Echo string as mentioned in the assignment
echo The number of files are $filecount and the number of matching lines are $linecount
