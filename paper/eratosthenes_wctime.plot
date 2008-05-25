set xlabel "Iterations"
set ylabel "Time (seconds)"
set autoscale
plot "aggregated/eratosthenes-01_wctime" with lines title "A", \
     "aggregated/eratosthenes-02_wctime" with lines title "B", \
     "aggregated/eratosthenes-03_wctime" with lines title "C", \
     "aggregated/eratosthenes-04_wctime" with lines title "D", \
     "aggregated/eratosthenes-05_wctime" with lines title "E", \
     "aggregated/eratosthenes-06_wctime" with lines title "F", \
     "aggregated/eratosthenes-07_wctime" with lines title "G", \
     "aggregated/eratosthenes-08_wctime" with lines title "H", \
     "aggregated/eratosthenes-09_wctime" with lines title "I";
