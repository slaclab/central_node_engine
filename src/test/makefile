
CENTRAL_NODE_ENGINE_DIR=..

include $(CENTRAL_NODE_ENGINE_DIR)/defs.mak
#INSTALL_DIR=/blah/blah

STATIC_LIBRARIES_YES+=
SHARED_LIBRARIES_YES+=

#rate_tst_SRCS       = rate_tst.cc
#rate_tst_LIBS       = $(CENTRAL_NODE_ENGINE_LIBS)
#TESTPROGRAMS            += rate_tst

#rate2_tst_SRCS       = rate2_tst.cc
#rate2_tst_LIBS       = $(CENTRAL_NODE_ENGINE_LIBS)
#TESTPROGRAMS            += rate2_tst

bcast_tst_SRCS = bcast_tst.cc
bcast_tst_LIBS =
TESTPROGRAMS  += bcast_tst

ln_tst_SRCS = ln_tst.cc
ln_tst_LIBS =
TESTPROGRAMS  += ln_tst

rateFirmware_tst_SRCS       = rateFirmware_tst.cc
rateFirmware_tst_LIBS       = $(CENTRAL_NODE_ENGINE_LIBS)
TESTPROGRAMS            += rateFirmware_tst

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

central_node_firmware_config_tst_SRCS       = central_node_firmware_config_tst.cc
central_node_firmware_config_tst_LIBS       = $(CENTRAL_NODE_ENGINE_LIBS)
TESTPROGRAMS            += central_node_firmware_config_tst

central_node_bypass_tst_SRCS       = central_node_bypass_tst.cc
central_node_bypass_tst_LIBS       = $(CENTRAL_NODE_ENGINE_LIBS)
TESTPROGRAMS            += central_node_bypass_tst

central_node_watchdog_tst_SRCS       = central_node_watchdog_tst.cc
central_node_watchdog_tst_LIBS       = $(CENTRAL_NODE_ENGINE_LIBS)
TESTPROGRAMS                        += central_node_watchdog_tst

central_node_watchdog_only_tst_SRCS  = central_node_watchdog_only_tst.cc
central_node_watchdog_only_tst_LIBS  = $(CENTRAL_NODE_ENGINE_LIBS)
TESTPROGRAMS                        += central_node_watchdog_only_tst

PROGRAMS = 
include $(CENTRAL_NODE_ENGINE_DIR)/rules.mak

# may override for individual test targets which will be
# executed with for all option sets, eg:
#
#    some_tst_run:RUN_OPTS='' '-a -b'
#
# (will run once w/o opts, a second time with -a -b)

