
CENTRAL_NODE_DIR=..

include $(CENTRAL_NODE_DIR)/defs.mak

DEP_HEADERS +=

STATIC_LIBRARIES_YES+=
SHARED_LIBRARIES_YES+=

rssi_bridge_SRCS = rssi_bridge.cc
rssi_bridge_SRCS+= udpsrv_util.cc
rssi_bridge_SRCS+= udpsrv_stream_state_monitor.cc

rssi_bridge_LIBS = $(CENTRAL_NODE_LIBS)

central_node_database_tst_SRCS       = central_node_database_tst.cc
central_node_database_tst_LIBS       = $(CENTRAL_NODE_LIBS)
TESTPROGRAMS            += central_node_database_tst

central_node_engine_tst_SRCS       = central_node_engine_tst.cc
central_node_engine_tst_LIBS       = $(CENTRAL_NODE_LIBS)
TESTPROGRAMS            += central_node_engine_tst

central_node_bypass_tst_SRCS       = central_node_bypass_tst.cc
central_node_bypass_tst_LIBS       = $(CENTRAL_NODE_LIBS)
TESTPROGRAMS            += central_node_bypass_tst

PROGRAMS =
include $(CENTRAL_NODE_DIR)/rules.mak

# may override for individual test targets which will be
# executed with for all option sets, eg:
#
#    some_tst_run:RUN_OPTS='' '-a -b'
#
# (will run once w/o opts, a second time with -a -b)

# run for V2 and V1 and over TDEST demuxer (v2)
# NOTE: the t1 test will be slow as the packetized channel is scrambled
#       by udpsrv (and this increases average roundtrip time)
#central_node_netio_tst_run:     RUN_OPTS=$(central_node_netio_tst_RUN_OPTS)

#central_node_srpmux_tst_run:    RUN_OPTS='' '-V1 -p8191' '-p8202 -r'
#central_node_netio_tst: udpsrv
