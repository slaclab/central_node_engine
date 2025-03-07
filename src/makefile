-include ../config.mak

ifdef BR_VERSION
	CXX	     = $(XCROSS_HOME)g++
	TARCH    = $(BR_VERSION)-x86_64
else
	CXX    = g++
	TARCH  = $(HARCH)
endif

OBJDIR   = O.$(TARCH)
SRCDIR   = .
LIBDIR   = O.$(TARCH)
INSTALL_DIR = ..

DEPS     =
SRCS     = $(wildcard $(SRCDIR)/*.cc)
OBJS     = $(patsubst $(SRCDIR)/%.cc, $(OBJDIR)/%.o, $(SRCS))
SLIB     = $(LIBDIR)/lib$(LIB_NAME).a
DLIB     = $(LIBDIR)/lib$(LIB_NAME).so
HEADERS  = $(wildcard $(SRCDIR)/*.h)

TESTDIR     = test
TESTBINDIR  = O.$(TARCH)
TESTSRCS    = $(wildcard $(TESTDIR)/*.cc)
TEST_TARGET = $(patsubst $(TESTDIR)/%.cc, $(TESTBINDIR)/%, $(TESTSRCS))

CXXFLAGS += -g -std=c++0x -fPIC
CXXFLAGS += $(addprefix -I,. $(CPSW_DIR)/include $(BOOST_DIR)/include $(YAML_CPP_DIR)/include) 

LIBS_EXT = $(addprefix -l,$(LIB_NAME) cpsw yaml-cpp boost_system $(TIRPC_LIBS_$(WITH_TIRPC_$(TARCH))) rt dl)
LIBS_SYS = -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -lrt -ldl

LDIR  = -L. 
LDIR += -L$(CPSW_DIR)/lib
LDIR += -L$(BOOST_DIR)/lib
LDIR += -L$(YAML_CPP_DIR)/lib
LDIR += -L$(TIRPC_DIR)/lib
LDIR += -L$(LIBDIR)

TIRPC_LIBS_YES = tirpc
WITH_TIRPC_rhel9-x86_64 = YES

.PHONY: all clean insatll uninstall

all: $(SLIB) $(DLIB) $(TEST_TARGET)

$(OBJDIR)/%.o : $(SRCDIR)/%.cc $(DEPS) | $(OBJDIR)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

$(OBJDIR):
	@mkdir -p $@

$(LIBDIR):
	@mkdir -p $@

$(SLIB): $(OBJS) | $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(DLIB): LDFLAGS += -shared
$(DLIB): $(OBJS) | $(LIBDIR)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(TESTBINDIR):
	@mkdir -p $@

$(TESTBINDIR)/%: $(TESTDIR)/%.cc
	$(CXX) -o $@ $^ -static $(CXXFLAGS) $(LDIR) $(LIBS_EXT) $(LIBS_SYS)

clean:
	$(RM) -rf $(OBJDIR)
	$(RM) -rf $(LIBDIR) 
	$(RM) -rf $(TESTBINDIR)

install: $(SLIB) $(DLIB) $(TEST_TARGET)
	@mkdir -p $(INSTALL_DIR)/$(TARCH)/lib
	install -C $(SLIB) $(INSTALL_DIR)/$(TARCH)/lib/
	install -C $(DLIB) $(INSTALL_DIR)/$(TARCH)/lib/
	
	@mkdir -p $(INSTALL_DIR)/$(TARCH)/include
	install -C $(HEADERS) $(INSTALL_DIR)/$(TARCH)/include/

	@mkdir -p $(INSTALL_DIR)/$(TARCH)/bin
	install -C $(TEST_TARGET) $(INSTALL_DIR)/$(TARCH)/bin/

uninstall:
	$(RM) -rf $(INSTALL_DIR)/$(TARCH)
