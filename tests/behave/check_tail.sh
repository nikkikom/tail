#!/bin/bash
# checks that output of the bin/tail program is same to output of system tail
PATH=/bin:/usr/bin:$PATH

lines=5

while getopts "n:m:r:" opt; do
  case $opt in
    n)
      lines=$OPTARG
      ;;

    r)
      returns=$OPTARG
      ;;

    m)
      method="-m $OPTARG"
      ;;
  esac
done

file=${@:$OPTIND:1}
if [ "x$file" == "x-" ]; then file=""; fi
#echo lines=$lines returns=${returns:-$lines} method=$method file=$file
cmp -s <(tail -n ${returns:-$lines} $file) <(../../bin/tail -n $lines $method $file)
exit
