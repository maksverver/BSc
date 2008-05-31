set xlabel "Iterations (x1000)"
set ylabel "Time (seconds)"
plot "aggregated/eratosthenes-01_wctime" with lines title "capacity=100,000", \
     "aggregated/eratosthenes-02_wctime" with lines title "capacity=1,000,000", \
     "aggregated/eratosthenes-03_wctime" with lines title "capacity=10,000,000";
