all: paper.pdf

%.pdf: %.tex %.bib
	pdflatex $*
	bibtex   $*
	pdflatex $*
	pdflatex $*

#paper.md.tex: paper.md
#	pandoc -f markdown+backtick_code_blocks+citations -t latex --listings --highlight-style=pygments -o paper.md.tex paper.md
