set xlabel "Iterations (x1000)"
set ylabel "Time (seconds)"
plot "aggregated/eratosthenes-07_wctime" with lines title "{/Symbol d}=0.125", \
     "aggregated/eratosthenes-08_wctime" with lines title "{/Symbol d}=0.25",  \
     "aggregated/eratosthenes-09_wctime" with lines title "{/Symbol d}=0.5",   \
     "aggregated/eratosthenes-10_wctime" with lines title "{/Symbol d}=0.667", \
     "aggregated/eratosthenes-11_wctime" with lines title "{/Symbol d}=0.75";
