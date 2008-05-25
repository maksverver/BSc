set xlabel "Iterations"
set ylabel "Queued States"
set autoscale
plot "results/eratosthenes-base-1" using 1:2 with lines title "Eratosthenes"
