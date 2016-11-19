
CENTRAL_NODE_ENGINE_DIR=.

include $(CENTRAL_NODE_ENGINE_DIR)/defs.mak

HEADERS = central_node_yaml.h central_node_database.h central_node_engine.h time_util.h

central_node_engine_SRCS = central_node_database.cc central_node_engine.cc central_node_bypass.cc time_util.cc
#central_node_engine_SRCS+= central_node_engine_entry_adapt.cc

STATIC_LIBRARIES+=central_node_engine

central_node_database_tst_SRCS = central_node_database_tst.cc
central_node_database_tst_LIBS = $(CENTRAL_NODE_ENGINE_LIBS)
central_node_database_tst_LIBS += central_node_engine
central_node_database_tst_LIBS += yaml-cpp
YAML_VERSION             = yaml-cpp-0.5.3
YAML_ARCH_linux_x86_64   = rhel6-x86_64
YAML_ARCH_linuxRT_x86_64 = buildroot-2015.02-x86_64
YAML_ARCH                = $(YAML_ARCH_$(TARNM))
yaml_DIR                 = $(PACKAGE_TOP)/yaml-cpp/$(YAML_VERSION)/$(YAML_ARCH)
yaml_cpp_DIR            += $(yaml_DIR)/lib
INCLUDE_DIRS            += $(yaml_DIR)/include
TESTPROGRAMS            += central_node_database_tst

central_node_engine_tst_SRCS = central_node_engine_tst.cc
central_node_engine_tst_LIBS = $(CENTRAL_NODE_ENGINE_LIBS)
central_node_engine_tst_LIBS += central_node_engine
central_node_engine_tst_LIBS += yaml-cpp
YAML_VERSION             = yaml-cpp-0.5.3
YAML_ARCH_linux_x86_64   = rhel6-x86_64
YAML_ARCH_linuxRT_x86_64 = buildroot-2015.02-x86_64
YAML_ARCH                = $(YAML_ARCH_$(TARNM))
yaml_DIR                 = $(PACKAGE_TOP)/yaml-cpp/$(YAML_VERSION)/$(YAML_ARCH)
yaml_cpp_DIR            += $(yaml_DIR)/lib
INCLUDE_DIRS            += $(yaml_DIR)/include
TESTPROGRAMS            += central_node_engine_tst

central_node_bypass_tst_SRCS = central_node_bypass_tst.cc
central_node_bypass_tst_LIBS = $(CENTRAL_NODE_ENGINE_LIBS)
central_node_bypass_tst_LIBS += central_node_engine
central_node_bypass_tst_LIBS += yaml-cpp
YAML_VERSION             = yaml-cpp-0.5.3
YAML_ARCH_linux_x86_64   = rhel6-x86_64
YAML_ARCH_linuxRT_x86_64 = buildroot-2015.02-x86_64
YAML_ARCH                = $(YAML_ARCH_$(TARNM))
yaml_DIR                 = $(PACKAGE_TOP)/yaml-cpp/$(YAML_VERSION)/$(YAML_ARCH)
yaml_cpp_DIR            += $(yaml_DIR)/lib
INCLUDE_DIRS            += $(yaml_DIR)/include
TESTPROGRAMS            += central_node_bypass_tst

include $(CENTRAL_NODE_ENGINE_DIR)/rules.mak

