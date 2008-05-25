set xlabel "Iterations (x1000)"
set ylabel "Time (seconds)"
plot "aggregated/eratosthenes-01_wctime" with lines title "capacity=100k", \
     "aggregated/eratosthenes-02_wctime" with lines title "capacity=1m", \
     "aggregated/eratosthenes-03_wctime" with lines title "capacity=10m";