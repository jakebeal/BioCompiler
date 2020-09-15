test_files_paleocompiler = \
	paleo-only/assoc.test \
	paleo-only/compiler.test \
	paleo-only/complex.test \
	paleo-only/emit-complex.test \
	paleo-only/mathlib.test \
	paleo-only/math.test \
	paleo-only/misc.test \
	paleo-only/tuple.test

test_files_neocompiler = \
	neo-only/emitter.test \
	neo-only/localization.test \
	neo-only/mathlib.test \
	neo-only/math.test \
	neo-only/neocompiler.test \
	neo-only/tuple.test

test_files_common = \
	universal/position.test \
	universal/smoke.test \
	universal/vectcomp.test
	universal/plugins.test

if USE_NEOCOMPILER
test_files = $(test_files_common) $(test_files_neocompiler)
else
test_files = $(test_files_common) $(test_files_paleocompiler)
endif

bin_SCRIPTS = prototest.py

# installed tests

installcheck-local:
	$(PYTHON) $(srcdir)/prototest.py \
		--proto=$(bindir)/proto \
		--p2b=$(bindir)/p2b \
		--demos=$(top_srcdir)/demos \
		`for t in $(test_files); do echo $(srcdir)/$$t; done`
	rm -rf dumps

# source-dir checks

check-local:
	$(PYTHON) $(srcdir)/prototest.py \
		--proto=$(top_builddir)/proto \
		--p2b=$(top_builddir)/p2b \
		--demos=$(top_srcdir)/demos \
		`for t in $(test_files); do echo $(srcdir)/$$t; done`
	rm -rf dumps

# cleanup

clean-local:
	rm -rf dumps

clean-tests:
	rm -rf */*.RESULTS
	rm -rf */xml/*.xml

