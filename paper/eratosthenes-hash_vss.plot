set xlabel "Iterations (x1000)"
set ylabel "Memory (MB)"
set key left top
plot "aggregated/eratosthenes-01_vss" with lines title "capacity=100,000", \
     "aggregated/eratosthenes-02_vss" with lines title "capacity=1,000,000", \
     "aggregated/eratosthenes-03_vss" with lines title "capacity=10,000,000";
