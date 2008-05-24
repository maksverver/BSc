FIGURES=memhier.eps

all: paper.dvi

paper.dvi: paper.tex paper.bib $(FIGURES)
	latex "$<"
	bibtex "`basename "$<" .tex`".aux
	latex "$<"
	latex "$<"

paper.pdf: paper.dvi
	sh -c 'for f in $(FIGURES); do epstopdf "$$f"; done'
	pdflatex paper.tex

paper.rtf: paper.dvi
	latex2rtf paper.tex

clean:
	rm -f paper.{aux,log,bbl,blg}

distclean: clean
	rm -f paper.dvi *.pdf

.PHONY: all clean distclean 

