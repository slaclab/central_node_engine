# host architecture
HARCH=host
# Architectures to build
ARCHES=$(HARCH)

# Save default (host) tools
CC_host    :=$(CC)
CXX_host   :=$(CXX)
AR_host    :=$(AR)
RANLIB_host:=$(or $(RANLIB),ranlib)

GIT_VERSION_STRING:=$(shell echo -n '"'`git describe --always --dirty`'"')

# if CROSS_xxx (for target architecture 'xxx' is not defined then try CROSS_default)
#
# For a host build this should be empty (since we assume the tools to be in PATH)
# but the user may override by defining CROSS_host...
#
# arch2var(VARIABLE_PREFIX)
define arch2var
$(1)=$$(or $$($(1)_$$(TARNM)),$$($(1)_default))
endef

# arch2libvar(VARIABLE_PREFIX)
define arch2libvar
$(1)_DIR=$$(or $$($(1)_DIR_$$(TARNM)),$$($(1)_DIR_default))
$(1)inc_DIR=$$(or $$($(1)inc_DIR_$$(TARNM)),$$($(1)inc_DIR_default),$$(addsuffix /include,$$($(1)_DIR)))
$(1)lib_DIR=$$(or $$($(1)lib_DIR_$$(TARNM)),$$($(1)lib_DIR_default),$$(addsuffix /lib,$$($(1)_DIR)))
endef

# When expanding any of the ARCHSPECIFIC_VARS
# we'll try $(var_$(TARNM)) and $(var_default)
ARCHSPECIFIC_VARS+=CROSS
ARCHSPECIFIC_VARS+=WITH_SHARED_LIBRARIES
ARCHSPECIFIC_VARS+=WITH_STATIC_LIBRARIES
ARCHSPECIFIC_VARS+=USR_CPPFLAGS
ARCHSPECIFIC_VARS+=USR_CXXFLAGS
ARCHSPECIFIC_VARS+=USR_CFLAGS
ARCHSPECIFIC_VARS+=USR_LDFLAGS

# For all of the ARCHSPECIFIC_LIBVARS
# we'll construct
# var_DIR, varinc_DIR, varlib_DIR
# and let each one try e.g.,
#
# $(<var>inc_DIR_$(TARNM)), $(<var>inc_DIR_default), $(<var>_DIR)/include
#
# (with $(<var>_DIR) also trying $(<var>_DIR_$(TARNM)), $(<var>_DIR_default))
#
ARCHSPECIFIC_LIBVARS+=cpsw
ARCHSPECIFIC_LIBVARS+=yaml_cpp
ARCHSPECIFIC_LIBVARS+=boost
ARCHSPECIFIC_LIBVARS+=py

$(foreach var,$(ARCHSPECIFIC_VARS),$(eval $(call arch2var,$(var))))

$(foreach var,$(ARCHSPECIFIC_LIBVARS),$(eval $(call arch2libvar,$(var))))

CROSS_default=$(addsuffix -,$(filter-out $(HARCH),$(TARCH)))

# Tools
CC     =$(CROSS)$(or $(CC_$(TARNM)),$(CC_default),gcc)
CXX    =$(CROSS)$(or $(CXX_$(TARNM)),$(CXX_default),g++)
AR     =$(CROSS)$(or $(AR_$(TARNM)),$(AR_default),ar)
RANLIB =$(CROSS)$(or $(RANLIB_$(TARNM)),$(RANLIB_default),ranlib)
INSTALL=install -C

# Tool options
OPT_CXXFLAGS=-g -Wall -O2
OPT_CFLAGS  =-g -Wall -O2

CPPFLAGS+= $(addprefix -I,$(SRCDIR) $(INCLUDE_DIRS) $(INSTALL_DIR:%=%/include))
CPPFLAGS+= $(addprefix -I,$(subst :, ,$(central_node_engineinc_DIRS)))
CPPFLAGS+= $(USR_CPPFLAGS)
CXXFLAGS+= $(OPT_CXXFLAGS)
CXXFLAGS+= $(USR_CXXFLAGS)
CFLAGS  += $(OPT_CFLAGS)
CFLAGS  += $(USR_CFLAGS)

LDFLAGS += $(USR_LDFLAGS)

VPATH=$(SRCDIR)

FILTERED_TBINS=$(filter-out $(DISABLED_TESTPROGRAMS) %.so,$(TESTPROGRAMS))

RUN_OPTS=''

# Libraries currently required by CENTRAL_NODE_ENGINE itself (and thus by anything using it)

# colon separated dirlist
# Note: += adds a whitespace
central_node_engineinc_DIRS=$(CENTRAL_NODE_ENGINE_DIR)$(addprefix :,$(cpswinc_DIR))$(addprefix :,$(boostinc_DIR))$(addprefix :,$(yaml_cppinc_DIR))
# colon separated dirlist
central_node_enginelib_DIRS=$(addsuffix /O.$(TARCH),$(CENTRAL_NODE_ENGINE_DIR))$(addprefix :,$(boostlib_DIR))$(addprefix :,$(yaml_cpplib_DIR))

# Libraries CENTRAL_NODE_ENGINE requires -- must be added to application's <prog>_LIBS variable
CENTRAL_NODE_ENGINE_LIBS   = central_node_engine cpsw yaml-cpp pthread rt dl

STATIC_LIBRARIES=$(STATIC_LIBRARIES_$(WITH_STATIC_LIBRARIES))
SHARED_LIBRARIES=$(SHARED_LIBRARIES_$(WITH_SHARED_LIBRARIES))

# Default values -- DO NOT OVERRIDE HERE but in config.mak or config.local.mak
WITH_SHARED_LIBRARIES_default=YES
WITH_STATIC_LIBRARIES_default=NO

COMMA__:=,
SPACE__:=
SPACE__+=

ifndef SRCDIR
SRCDIR=.
endif

ifndef UPDIR
TOPDIR=./
else
TOPDIR=$(UPDIR)
endif

# definitions
include $(CENTRAL_NODE_ENGINE_DIR)/../config.mak
-include $(CENTRAL_NODE_ENGINE_DIR)/../config.local.mak
-include $(SRCDIR)/local.mak
