set title "Fibonacci"
set xlabel "size"
set ylabel "time (ns)"
set terminal png font " Times_New_Roman,12 "
set terminal png size 1024,768
set output "result.png"
set xtics 0 ,10 ,100
set key left

plot \
"result.dat" using 0:2 with linespoints linewidth 2 title "user", \
"result.dat" using 0:3 with linespoints linewidth 2 title "kernel", \
"result.dat" using 0:4 with linespoints linewidth 2 title "kernel to user", \
