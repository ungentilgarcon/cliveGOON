all: paper.pdf talk.pdf getting-started.pdf talk-thsf.pdf lightning-talk.pdf

clean:
	-rm paper.aux paper.bbl paper.blg paper.log paper.out

%.pdf: %.tex %.bib
	pdflatex $*
	bibtex   $*
	pdflatex $*
	pdflatex $*

%.pdf: %.md
	pandoc --slide-level 2 -t beamer $*.md -o $*.pdf

#paper.md.tex: paper.md
#	pandoc -f markdown+backtick_code_blocks+citations -t latex --listings --highlight-style=pygments -o paper.md.tex paper.md
