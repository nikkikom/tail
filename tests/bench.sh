#!/bin/bash

# return timings
function get_times {
  maxrun=$1
  time=$(time (for i in $(seq 1 $maxrun); do $2; done >/dev/null) 2>&1) || exit
  time=($time)
  echo ${time[1]} ${time[3]} ${time[5]}
}

# convert time to float point
function to_float {
  [[ "$2" =~ ([0-9]+)m([0-9]{1,2}\.[0-9]+)s ]] || exit
  echo $(bc -l <<< "(${BASH_REMATCH[1]}*60+${BASH_REMATCH[2]})*1.0/$1")
}

gen_text=../bin/gen_text
tail=../bin/tail

# lines number min:max:delta
arg_lines=1:10000:500

# line length
arg_lengths=1:100000:10000

# methods
arg_methods=mmap,read,pipe

while getopts "n:m:r:" opt; do
case $opt in
    n)
      arg_lines=$OPTARG
      ;;

    l)
      arg_lengths=$OPTARG
      ;;

    m)
      arg_methods=$OPTARG
      ;;
  esac
done

file=${@:$OPTIND:1}
file=${file:-bench.test}

IFS=": " read lines_min lines_max lines_delta <<< $arg_lines
IFS=": " read len_min len_max len_delta <<< $arg_lengths
IFS=", " read -a methods <<< $arg_methods

# how many times we will run same test
runs=3

# clear data files
for method in "${methods[@]}"; do
  echo '"lines" "length" "real" "user" "sys"' > "bench_results.$method"
done

# generate all the input files and run the tail
for line in $(seq $lines_min $lines_delta $lines_max); do
  for len in $(seq $len_min $len_delta $len_max); do
  $gen_text random:$line:$len > $file
    # warming up
    cat "$file" > /dev/null
    echo -n .

    for method in "${methods[@]}"; do
      times=($(get_times $runs "$tail -n 5 -m $method $file"))

      echo $line $len $(to_float $runs ${times[0]}) $(to_float $runs ${times[1]}) $(to_float $runs ${times[2]}) >> "bench_results.$method"
    done
  done

  for method in "${methods[@]}"; do
    (echo; echo) >> "bench_results.$method"
  done

  echo
done
/bin/rm -f ./"$file"
echo

# plot
for method in "${methods[@]}"; do
  cat > bench_results.$method.gnuplot <<EOF
set title "Tail in mode '$method'"
set key autotitle columnheader
set xlabel "Lines in file" left rotate parallel offset 5
set ylabel "Line length\n(x1000)" right rotate parallel offset 0,-0.8
set ytics offset 0,-0.5
set zlabel "Execution time (sec)" rotate left
set xyplane relative 0
set view 60,300
set dgrid3d 30,30 qnorm 2
set term png transparent nocrop enhanced size 480,320 font "arial,7"
set output "bench_results.$method.png"
splot 'bench_results.$method' u 1:2/1000:3 w l palette
EOF
  gnuplot < bench_results.$method.gnuplot
done
