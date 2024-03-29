set xlabel "Iterations (x1000)"
set ylabel "Memory (MB)"
set key left top
plot "aggregated/eratosthenes-03_vss" with lines title "Hash table", \
     "aggregated/eratosthenes-06_vss" with lines title "B-tree", \
     "aggregated/eratosthenes-09_vss" with lines title "Bender set";
