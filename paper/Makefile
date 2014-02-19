PROJECT = paper

all: $(PROJECT)

$(PROJECT): $(PROJECT).tex ../common/acronyms.tex ../common/bibliography.bib
	pdflatex $(PROJECT).tex
	bibtex $(PROJECT).aux
	pdflatex $(PROJECT).tex
	pdflatex $(PROJECT).tex

clean:
	rm -f $(PROJECT)-blx.bib $(PROJECT).{aux,bbl,blg,log,out} $(PROJECT).run.xml
