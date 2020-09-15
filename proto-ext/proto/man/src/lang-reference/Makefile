PROJ_NAME=proto-language-reference
PROJ_NAME_2=proto-quick-start

all:lang quick

lang:
	pdflatex ${PROJ_NAME}.tex
	pdflatex ${PROJ_NAME}.tex

quick:
	pdflatex ${PROJ_NAME_2}.tex
	pdflatex ${PROJ_NAME_2}.tex

clean:lang-clean quick-clean

lang-clean:
	-rm ${PROJ_NAME}.aux
	-rm ${PROJ_NAME}.log
	-rm ${PROJ_NAME}.pdf

quick-clean:
	-rm ${PROJ_NAME_2}.aux
	-rm ${PROJ_NAME_2}.log
	-rm ${PROJ_NAME_2}.pdf
