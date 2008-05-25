set xlabel "Iterations"
set ylabel "Memory (bytes)"
set autoscale
plot "aggregated/eratosthenes-07_vss" with lines title "density=0.125", \
     "aggregated/eratosthenes-08_vss" with lines title "density=0.25", \
     "aggregated/eratosthenes-09_vss" with lines title "density=0.5";
