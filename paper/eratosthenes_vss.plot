set xlabel "Iterations (x1000)"
set ylabel "Memory (MB)"
plot "aggregated/eratosthenes-01_vss" with lines title "A", \
     "aggregated/eratosthenes-02_vss" with lines title "B", \
     "aggregated/eratosthenes-03_vss" with lines title "C", \
     "aggregated/eratosthenes-04_vss" with lines title "D", \
     "aggregated/eratosthenes-05_vss" with lines title "E", \
     "aggregated/eratosthenes-06_vss" with lines title "F", \
     "aggregated/eratosthenes-07_vss" with lines title "G", \
     "aggregated/eratosthenes-08_vss" with lines title "H", \
     "aggregated/eratosthenes-09_vss" with lines title "I";
