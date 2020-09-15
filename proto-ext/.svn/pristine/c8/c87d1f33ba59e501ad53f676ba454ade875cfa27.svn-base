SUBDIRS = html

man3dir = $(mandir)/man3
docdir  = $(prefix)/doc/@PACKAGE_TARNAME@-@PACKAGE_VERSION@

EXTRA_DIST = \
	mainPage.txt

.PHONY: dox pdf

all-local: dox #pdf

dox: html html/api/index.html
html/api/index.html: Doxyfile
	@DOXYGEN@

pdf: @PACKAGE_TARNAME@.pdf
@PACKAGE@.pdf:
	$(MAKE) -C ./latex pdf
	ln -s ./latex/refman.ps	 @PACKAGE_TARNAME@.ps
	ln -s ./latex/refman.pdf @PACKAGE_TARNAME@.pdf

clean-local:
	$(RM) -r latex
	$(RM) -r html/api man @PACKAGE_TARNAME@.ps @PACKAGE_TARNAME@.pdf
