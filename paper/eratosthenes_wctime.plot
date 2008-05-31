set xlabel "Iterations"
set ylabel "Time (seconds)"
plot "aggregated/eratosthenes-03_wctime" with lines title "Hash table", \
     "aggregated/eratosthenes-06_wctime" with lines title "B-tree", \
     "aggregated/eratosthenes-09_wctime" with lines title "Bender set";
