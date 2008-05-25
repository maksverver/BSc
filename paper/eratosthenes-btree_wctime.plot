set xlabel "Iterations (x1000)"
set ylabel "Time (seconds)"
plot "aggregated/eratosthenes-04_wctime" with lines title "pagesize=1KB", \
     "aggregated/eratosthenes-05_wctime" with lines title "pagesize=4KB", \
     "aggregated/eratosthenes-06_wctime" with lines title "pagesize=16KB";