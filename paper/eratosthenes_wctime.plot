set xlabel "Iterations"
set ylabel "Time (seconds)"
plot "aggregated/eratosthenes-01_wctime" with lines title "A", \
     "aggregated/eratosthenes-02_wctime" with lines title "B", \
     "aggregated/eratosthenes-03_wctime" with lines title "C";