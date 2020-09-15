noinst_LTLIBRARIES = liboptimize.la

AM_CPPFLAGS = \
        -I$(top_srcdir)

liboptimize_la_LDFLAGS = -dynamiclib -avoid-version

liboptimize_la_SOURCES = \
	ConstantEliminator.cpp \
	CopyPropagator.cpp \
	DeadCodeEliminator.cpp \
	DoubleNegativeEliminator.cpp \
	DuplicateReactionConsolidator.cpp \
	InferChemicalType.cpp \
	MergeDuplicateInputs.cpp \
	MultiProductEliminator.cpp \
	NorCompressor.cpp \
	OutputConsolidator.cpp \
	grn_optimizers.cpp
