set xlabel "Iterations (x1000)"
set ylabel "Memory (MB)"
plot "aggregated/eratosthenes-04_vss" with lines title "pagesize=1KB", \
     "aggregated/eratosthenes-05_vss" with lines title "pagesize=4KB", \
     "aggregated/eratosthenes-06_vss" with lines title "pagesize=16KB";