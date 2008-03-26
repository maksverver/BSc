FIGURES=memhier.eps

proposal.dvi: proposal.tex proposal.bib $(FIGURES)
	latex "$<"
	bibtex "`basename "$<" .tex`".aux
	latex "$<"
	latex "$<"

proposal.pdf: proposal.dvi
	sh -c 'for f in $(FIGURES); do epstopdf "$$f"; done'
	pdflatex proposal.tex

proposal.rtf: proposal.dvi
	latex2rtf proposal.tex

clean:
	rm -f proposal.{aux,log,bbl,blg}

distclean: clean
	rm -f proposal.dvi *.pdf

.PHONY: all clean distclean 

