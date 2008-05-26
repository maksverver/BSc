set xlabel "Iterations (x1000)"
set ylabel "Transitions (x1000)"
plot "aggregated/eratosthenes-base_trans" with lines title "Eratosthenes", \
     "aggregated/leader2-base_trans"      with lines title "Leader2",      \
     "aggregated/peterson-base_trans"     with lines title "Petersion_N";
