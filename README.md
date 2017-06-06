Enable/Disable Log
------------------

This package is currently using easylogging++ for debug messages. It can be enabled by setting this line in the defs.mak file:

     CXXFLAGS+= -DLOG_ENABLED

The flag must be taken out when compiling for LinuxRT. The LinuxRT version is compiled if the following is present in the config.mak file:

    ARCHES += linuxRT-x86_64

Tests
-----

The test subdirectory contains several *_tst.cc files that are stand alone testing programs:

Reads the contents of YAML MPS Database, configure/check it and prints out the contents:
     
     central_node_database_tst -f yaml/mps_gun_config.yaml -d

     Usage: ./O.linux-x86_64/central_node_database_tst [-f <file>] [-d]
       	    -f <file>   :  MPS database YAML file
       	    -d          :  dump YAML file to stdout
       	    -t          :  trace output
       	    -h          :  print this message

Evaluate digital/analog inputs (from files) using the rules/inputs described in YAML MPS Database:

     central_node_engine_tst -f yaml/sxrss.yaml  -i inputs/sxrss_digital_inputs.txt

     Usage: ./O.linux-x86_64/central_node_engine_tst -f <file> -i <file>
            -f <file>   :  MPS database YAML file
       	    -i <file>   :  digital input test file
       	    -a <file>   :  analog input test file
       	    -v          :  verbose output
       	    -t          :  trace output
       	    -h          :  print this message
