clive.pdf: clive.tex clive.bib
	pdflatex clive
	bibtex   clive
	pdflatex clive
	pdflatex clive

#clive.md.tex: clive.md
#	pandoc -f markdown+backtick_code_blocks+citations -t latex --listings --highlight-style=pygments -o clive.md.tex clive.md
