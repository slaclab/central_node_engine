
CENTRAL_NODE_ENGINE_DIR=..

include $(CENTRAL_NODE_ENGINE_DIR)/defs.mak
#INSTALL_DIR=/blah/blah

STATIC_LIBRARIES_YES+=
SHARED_LIBRARIES_YES+=

central_node_database_tst_SRCS       = central_node_database_tst.cc
central_node_database_tst_LIBS       = $(CENTRAL_NODE_ENGINE_LIBS)
TESTPROGRAMS            += central_node_database_tst

central_node_config_tst_SRCS       = central_node_config_tst.cc
central_node_config_tst_LIBS       = $(CENTRAL_NODE_ENGINE_LIBS)
TESTPROGRAMS            += central_node_config_tst

central_node_engine_tst_SRCS       = central_node_engine_tst.cc
central_node_engine_tst_LIBS       = $(CENTRAL_NODE_ENGINE_LIBS)
TESTPROGRAMS            += central_node_engine_tst

central_node_engine_server_SRCS       = central_node_engine_server.cc
central_node_engine_server_LIBS       = $(CENTRAL_NODE_ENGINE_LIBS)
TESTPROGRAMS            += central_node_engine_server

central_node_firmware_tst_SRCS       = central_node_firmware_tst.cc
central_node_firmware_tst_LIBS       = $(CENTRAL_NODE_ENGINE_LIBS)
TESTPROGRAMS            += central_node_firmware_tst

central_node_bypass_tst_SRCS       = central_node_bypass_tst.cc
central_node_bypass_tst_LIBS       = $(CENTRAL_NODE_ENGINE_LIBS)
TESTPROGRAMS            += central_node_bypass_tst

PROGRAMS = 
include $(CENTRAL_NODE_ENGINE_DIR)/rules.mak

# may override for individual test targets which will be
# executed with for all option sets, eg:
#
#    some_tst_run:RUN_OPTS='' '-a -b'
#
# (will run once w/o opts, a second time with -a -b)

