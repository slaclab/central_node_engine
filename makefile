
CENTRAL_NODE_DIR=.

include $(CENTRAL_NODE_DIR)/defs.mak

POSTBUILD_SUBDIRS=test

HEADERS = central_node_bypass.h central_node_bypass_manager.h \
	central_node_database.h central_node_engine.h central_node_exception.h \
	central_node_yaml.h log.h log_wrapper.h

GENERATED_SRCS += git_version_string.h
#GENERATED_SRCS += README.yamlDefinition
#GENERATED_SRCS += README.yamlDefinition.html
#GENERATED_SRCS += README.configData.html
#GENERATED_SRCS += INSTALL.html

central_node_engine_SRCS = central_node_database.cc
central_node_engine_SRCS += central_node_engine.cc
central_node_engine_SRCS += central_node_bypass.cc
central_node_engine_SRCS += time_util.cc

DEP_HEADERS  = $(HEADERS)
DEP_HEADERS += central_node_yaml.h
DEP_HEADERS += log.h
DEP_HEADERS += log_wrapper.h

STATIC_LIBRARIES_YES+=central_node_engine
SHARED_LIBRARIES_YES+=central_node_engine

#central_node_yaml_xpand_SRCS += central_node_yaml_xpand.cc
#central_node_yaml_xpand_LIBS += $(CENTRAL_NODE_LIBS)

#central_node_ypp_SRCS        += central_node_ypp.cc central_node_preproc.cc

# Python wrapper; only built if WITH_PYCENTRAL_NODE is set to YES (can be target specific)
#pycentral_node_so_SRCS    = central_node_python.cc
#pycentral_node_so_LIBS    = $(BOOST_PYTHON_LIB) $(CENTRAL_NODE_LIBS)
#pycentral_node_so_CPPFLAGS= $(addprefix -I,$(pyinc_DIR))
#pycentral_node_so_CXXFLAGS= -fno-strict-aliasing

#PYCENTRAL_NODE_YES        = pycentral_node.so
#PYCENTRAL_NODE            = $(PYCENTRAL_NODE_$(WITH_PYCENTRAL_NODE))

#PROGRAMS         += central_node_yaml_xpand central_node_ypp $(PYCENTRAL_NODE)

include $(CENTRAL_NODE_DIR)/rules.mak

#README.yamlDefinition:README.yamlDefinition.in
#	awk 'BEGIN{ FS="[ \t\"]+" } /\<YAML_KEY_/{ printf("s/%s/%s/g\n", $$2, $$3); }' central_node_yaml_keydefs.h | sed -f - $< > $@

#%.html: %
#	sed -e 's/&/\&amp/g' -e 's/</\&lt/g' -e 's/>/\&gt/g' $< | sed -e '/THECONTENT/{r /dev/stdin' -e 'd}' tmpl.html > $@

#test: sub-./test@test
#	@true
