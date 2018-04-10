all: paper.pdf

clean:
	-rm paper.aux paper.bbl paper.blg paper.log paper.out

%.pdf: %.tex %.bib
	pdflatex $*
	bibtex   $*
	pdflatex $*
	pdflatex $*

#paper.md.tex: paper.md
#	pandoc -f markdown+backtick_code_blocks+citations -t latex --listings --highlight-style=pygments -o paper.md.tex paper.md
