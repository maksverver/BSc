PLOTS=fig-queuesize.tex \
      fig-eratosthenes_vss.tex fig-eratosthenes_wctime.tex \
      fig-eratosthenes-bender_vss.tex fig-eratosthenes-bender_wctime.tex
FIGURES=memhier.eps bender-set.eps $(PLOTS)

all: paper.dvi

paper.dvi: paper.tex paper.bib $(FIGURES)
	latex "$<"
	bibtex "`basename "$<" .tex`".aux
	latex "$<"
	latex "$<"

$(PLOTS): fig-%.tex: %.plot
	gnuplot pstex.plot $< >$@

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

