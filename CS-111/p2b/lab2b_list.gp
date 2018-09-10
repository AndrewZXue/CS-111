#! /usr/bin/gnuplot
#
# purpose:
#     generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#    1. test name
#    2. # threads
#    3. # iterations per thread
#    4. # lists
#    5. # operations performed (threads x iterations x (ins + lookup + delete))
#    6. run time (ns)
#    7. run time per operation (ns)
#    8. wait time per operation (ns)
#
# output:
#    lab2b_list_1.png ... throughput vs. number of threads for mutex and spin-lock synchronized list operations.
#    lab2b_list_2.png ... mean time per mutex wait and mean time per operation for mutex-synchronized list operations.
#    lab2b_list_3.png ... successful iterations vs. threads for each synchronization method.
#    lab2b_list_4.png ... throughput vs. number of threads for mutex synchronized partitioned lists.
#    lab2b_list_5.png ... throughput vs. number of threads for spin-lock-synchronized partitioned lists.
#
# Note:
#    Managing data is simplified by keeping all of the results in a single
#    file.  But this means that the individual graphing commands have to
#    grep to select only the data they want.
#

# general plot parameters
set terminal png
set datafile separator ","

# 1
set title "lab2b_1: Throughput vs Threads Sync = m/s"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_1.png'

plot \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000)/($7) \
     title 'mutex' with linespoints lc rgb 'blue', \
     "< grep -e 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000)/($7) \
     title 'spin-lock' with linespoints lc rgb 'red'

# 2
set title "lab2b_2: time vs threads"
set xlabel "Threads"
set logscale x 2
set ylabel "Time(ns)"
set logscale y 10
set output 'lab2b_2.png'
# note that unsuccessful runs should have produced no output
plot \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
     title 'average time per operation (mutex)' with linespoints lc rgb 'blue', \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
     title 'lock delay (mutex)' with linespoints lc rgb 'red'
 
# 3    
set title "lab2b_3: itertations vs threads"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Iterations"
set logscale y 10
set output 'lab2b_3.png'
set key left top
plot \
     "< grep list-id-none lab2b_list.csv" using ($2):($3) \
     title 'no sync' with points lc rgb 'red', \
     "< grep list-id-m lab2b_list.csv" using ($2):($3) \
     title 'mutex' with points lc rgb 'blue', \
     "< grep list-id-s lab2b_list.csv" using ($2):($3) \
     title 'spin-lock' with points lc rgb 'green'


# 4
set title "lab2b.4: throughput vs number of threads."
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_4.png'
set key left top
plot \
     "< grep -e 'list-none-m,[0-9]2*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title '--lists = 1' with linespoints lc rgb 'red', \
     "< grep -e 'list-none-m,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title '--lists = 4' with linespoints lc rgb 'blue', \
     "< grep -e 'list-none-m,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title '--lists = 8' with linespoints lc rgb 'green', \
     "< grep -e 'list-none-m,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title '--list = 16' with linespoints lc rgb 'yellow'

# lab2b_5.png
set title "lab2b.5: Throughput vs threads Number for\nSpin-lock-synchronized Partitioned Lists."
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Operations per Second"
set logscale y 10
set output 'lab2b_5.png'
set key left top
plot \
     "< grep -e 'list-none-s,[0-9]2*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title '--list = 1' with linespoints lc rgb 'red', \
     "< grep -e 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title '--list = 4' with linespoints lc rgb 'blue', \
     "< grep -e 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title '--list = 8' with linespoints lc rgb 'green', \
     "< grep -e 'list-none-s,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title '--list = 16' with linespoints lc rgb 'yellow'






