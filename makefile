
CENTRAL_NODE_DIR=.

include $(CENTRAL_NODE_DIR)/defs.mak

POSTBUILD_SUBDIRS=test

HEADERS = central_node_bypass.h central_node_bypass_manager.h \
	central_node_database.h central_node_engine.h central_node_exception.h \
	central_node_yaml.h log.h log_wrapper.h time_util.h

GENERATED_SRCS += git_version_string.h

central_node_engine_SRCS = central_node_database.cc
central_node_engine_SRCS += central_node_inputs.cc
central_node_engine_SRCS += central_node_engine.cc
central_node_engine_SRCS += central_node_bypass.cc
central_node_engine_SRCS += time_util.cc

DEP_HEADERS  = $(HEADERS)
DEP_HEADERS += central_node_yaml.h
DEP_HEADERS += log.h
DEP_HEADERS += log_wrapper.h

STATIC_LIBRARIES_YES+=central_node_engine
SHARED_LIBRARIES_YES+=central_node_engine

include $(CENTRAL_NODE_DIR)/rules.mak


