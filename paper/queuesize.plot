set xlabel "Iterations (x1000)"
set ylabel "Queuesize (x1000)"
plot "aggregated/eratosthenes-base_qsz" with lines title "Eratosthenes", \
     "aggregated/leader2-base_qsz"      with lines title "Leader2",      \
     "aggregated/peterson-base_qsz"     with lines title "Petersion_N";
