set xlabel "Iterations (x1000)"
set ylabel "Time (seconds)"
set key left top
plot "aggregated/eratosthenes-04_wctime" with lines title "pagesize=1 KB", \
     "aggregated/eratosthenes-05_wctime" with lines title "pagesize=4 KB", \
     "aggregated/eratosthenes-06_wctime" with lines title "pagesize=16 KB";
