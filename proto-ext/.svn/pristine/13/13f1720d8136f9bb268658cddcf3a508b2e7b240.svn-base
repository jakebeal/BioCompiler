# NOTE: order is important in this list!

if USE_NEOCOMPILER
neopaleodir = neo
else
neopaleodir = paleo
endif

SUBDIRS = \
	core \
	$(neopaleodir) \
	.

protolib_DATA = \
	*.proto \
	core.ops

EXTRA_DIST = $(protolib_DATA)
