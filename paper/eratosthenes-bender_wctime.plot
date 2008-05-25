set xlabel "Iterations"
set ylabel "Time (seconds)"
set autoscale
plot "aggregated/eratosthenes-07_wctime" with lines title "density=0.125", \
     "aggregated/eratosthenes-08_wctime" with lines title "density=0.25", \
     "aggregated/eratosthenes-09_wctime" with lines title "density=0.5";
