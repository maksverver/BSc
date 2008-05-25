set xlabel "Iterations (x1000)"
set ylabel "Memory (MB)"
plot "aggregated/eratosthenes-01_vss" with lines title "capacity=100k", \
     "aggregated/eratosthenes-02_vss" with lines title "capacity=1m", \
     "aggregated/eratosthenes-03_vss" with lines title "capacity=10m";