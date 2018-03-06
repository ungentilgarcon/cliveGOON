all: clive.pdf

%.pdf: %.tex %.bib
	pdflatex $*
	bibtex   $*
	pdflatex $*
	pdflatex $*

#clive.md.tex: clive.md
#	pandoc -f markdown+backtick_code_blocks+citations -t latex --listings --highlight-style=pygments -o clive.md.tex clive.md
