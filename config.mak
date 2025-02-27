###################
### Build Flags ###
###################
# Enable this flag to allow CPSW to communicate with firmware, if not enable
# then the engine will run with inputs from the central_node_test.py script (mps_database package)
CXXFLAGS+= -DFW_ENABLED

# Log can't be enabled when building for linuxRT, disable them both for production build
#CXXFLAGS+= -DLOG_ENABLED
#CXXFLAGS+= -DLOG_STDOUT

################
### Lib name ###
################
LIB_NAME = central_node_engine

#####################
### Arch Versions ###
#####################
# Host version
#HARCH      = rhel6
# Buildroot versions
BR_ARCHES += buildroot-2019.08

########################
### Package versions ###
########################
CPSW_VERSION        = R4.5.2
BOOST_VERSION       = 1.64.0
YAML_CPP_VERSION    = yaml-cpp-0.5.3_boost-1.64.0
EASYLOGGING_VERSION = easyloggingpp-8.91
TIRPC_VERSION       = 1.3.5

########################
### Path definitions ###
########################
# Set a directory for the toolchain location. in S3DF, this is the same as EPICS_PACKAGE_TOP
ifeq ($(EPICS_PACKAGE_TOP),)
TOOLCHAINS=/afs/slac/package
else
TOOLCHAINS=$(EPICS_PACKAGE_TOP)
endif

# Location of BuildRoot home:
BUILDROOT_HOME=$(TOOLCHAINS)/linuxRT/$(BR_VERSION)
# Location CrossCompiler HOME:
XCROSS_HOME=$(BUILDROOT_HOME)/host/linux-x86_64/x86_64/usr/bin/x86_64-buildroot-linux-gnu-

ifeq ($(PACKAGE_SITE_TOP),)
$(error PACKAGE_SITE_TOP is not defined! Please set it in the environment or otherwise)
endif

# Package top
PACKAGE_TOP      = $(PACKAGE_SITE_TOP)
# Packages location
CPSW_DIR          = $(PACKAGE_TOP)/cpsw/framework/$(CPSW_VERSION)/$(TARCH)
BOOST_DIR         = $(PACKAGE_TOP)/boost/$(BOOST_VERSION)/$(TARCH)
YAML_CPP_DIR      = $(PACKAGE_TOP)/yaml-cpp/$(YAML_CPP_VERSION)/$(TARCH)
TIRPC_DIR         = $(PACKAGE_TOP)/tirpc/$(TIRPC_VERSION)/$(TARCH)
easyloggingpp_DIR = $(PACKAGE_TOP)/easylogging/$(EASYLOGGINGPP_VERSION)
