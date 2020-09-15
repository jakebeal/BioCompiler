INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/shared \
	-I$(top_srcdir)/src/compiler \
	-I$(top_srcdir)/src/sim \
	-I$(top_srcdir)/src/vm

defops_DATA = \
	platform_ops.proto \
	PerfectLocalizer.proto \
	SimpleDynamics.proto \
	simple-life-cycle.proto \
	mote-io.proto
defopsdir = $(protoplatdir)/sim/
EXTRA_DIST = $(defops_DATA)

# Basic plugins
pkglib_LTLIBRARIES = libdistributions.la libsimplelifecycle.la libmica2mote.la libradiomodels.la libstopwhen.la
pluginflags = -dynamiclib -avoid-version
pluginlibs = ../sim/libprotosimplugin.la
# On non-Mac systems, might need to have -dynamiclib removed, -module added, or other such changes

# shared library for distributions
libdistributions_la_SOURCES = \
	DistributionsPlugin.cpp
libdistributions_la_LIBADD = $(pluginlibs)
libdistributions_la_LDFLAGS = $(pluginflags)

# shared library for SimpleLifeCycle plugin
libsimplelifecycle_la_SOURCES = \
	SimpleLifeCyclePlugin.cpp
libsimplelifecycle_la_LIBADD = $(pluginlibs)
libsimplelifecycle_la_LDFLAGS = $(pluginflags)

# shared library for Mica2Motes plugin
libmica2mote_la_SOURCES = \
	Mica2MotePlugin.cpp
libmica2mote_la_LIBADD = $(pluginlibs)
libmica2mote_la_LDFLAGS = $(pluginflags)

# shared library for radio plugin
libradiomodels_la_SOURCES = \
	wormhole-radio.cpp \
	graph_link_radio.cpp \
	multiradio.cpp \
	RadioModelsPlugin.cpp
libradiomodels_la_LIBADD = $(pluginlibs)
libradiomodels_la_LDFLAGS = $(pluginflags)

# shared library for program-controlled stopping
libstopwhen_la_SOURCES = \
	stop-when.cpp
libstopwhen_la_LIBADD = $(pluginlibs)
libstopwhen_la_LDFLAGS = $(pluginflags)
