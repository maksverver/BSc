PLOTS=fig-queuesize.eps fig-transitions.eps \
      fig-eratosthenes_vss.eps fig-eratosthenes_wctime.eps \
      fig-eratosthenes-bender_vss.eps fig-eratosthenes-bender_wctime.eps \
      fig-eratosthenes-hash_vss.eps fig-eratosthenes-hash_wctime.eps \
      fig-eratosthenes-btree_vss.eps fig-eratosthenes-btree_wctime.eps \
      fig-leader2_vss.eps fig-leader2_wctime.eps \
      fig-peterson_vss.eps fig-peterson_wctime.eps
FIGURES=memhier.eps bender-set.eps $(PLOTS)

all: paper.dvi

paper.dvi: paper.tex paper.bib $(FIGURES)
	latex "$<"
	bibtex "`basename "$<" .tex`".aux
	latex "$<"
	latex "$<"

$(PLOTS): fig-%.eps: %.plot
	gnuplot eps.plot $< >$@

paper.pdf: paper.dvi
	sh -c 'for f in $(FIGURES); do epstopdf "$$f"; done'
	pdflatex paper.tex

paper.rtf: paper.dvi
	latex2rtf paper.tex

clean:
	rm -f paper.{log,bbl,blg} *.aux

distclean: clean
	rm -f paper.dvi *.pdf $(PLOTS)

.PHONY: all clean distclean 

