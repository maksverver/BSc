set xlabel "Iterations (x1000)"
set ylabel "Memory (MB)"
plot "aggregated/eratosthenes-07_vss" with lines title "{/Symbol d}=0.125", \
     "aggregated/eratosthenes-08_vss" with lines title "{/Symbol d}=0.25", \
     "aggregated/eratosthenes-09_vss" with lines title "{/Symbol d}=0.5";
