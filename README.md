Enable/Disable Log
------------------

This package is currently using easylogging++ for debug messages. It can be enabled by setting this line in the defs.mak file:

     CXXFLAGS+= -DLOG_ENABLED

The flag must be taken out when compiling for LinuxRT. The LinuxRT version is compiled if the following is present in the config.mak file:

    ARCHES += linuxRT-x86_64
