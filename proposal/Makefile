
proposal.dvi: proposal.tex
	latex "$<"
	bibtex "`basename "$<" .tex`".aux
	latex "$<"
	latex "$<"

proposal.pdf: proposal.dvi
	pdflatex proposal.tex

proposal.rtf: proposal.dvi
	latex2rtf proposal.tex

clean:
	rm -f proposal.{aux,log,bbl,blg}

distclean: clean
	rm -f proposal.{dvi,pdf}

.PHONY: all clean distclean 

