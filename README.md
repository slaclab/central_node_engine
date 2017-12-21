Requirements
------------

central_node_engine uses the following libraries from $PACKAGE_TOP:

'''
  yaml-cpp
  boost
  cpsw
  easylogging (for log messages - not working for LinuxRT yet see below how to disable)
'''

Building
--------

'''
$ make install
'''

Enable/Disable Log
------------------

This package is currently using easylogging++ for debug messages. It can be enabled by setting this line in the defs.mak file:

'''
     CXXFLAGS+= -DLOG_ENABLED
'''

The flag must be taken out when compiling for LinuxRT. The LinuxRT version is compiled if the following is present in the config.mak file:
'''
    ARCHES += linuxRT-x86_64
'''
Tests
-----

The test subdirectory contains several *_tst.cc files that are stand alone testing programs:

Reads the contents of YAML MPS Database, configure/check it and prints out the contents:

'''     
     central_node_database_tst -f yaml/mps_gun_config.yaml -d

     Usage: ./O.linux-x86_64/central_node_database_tst [-f <file>] [-d]
       	    -f <file>   :  MPS database YAML file
       	    -d          :  dump YAML file to stdout
       	    -t          :  trace output
       	    -h          :  print this message
'''

Evaluate digital/analog inputs (from files) using the rules/inputs described in YAML MPS Database:

'''
     central_node_engine_tst -f yaml/sxrss.yaml  -i inputs/sxrss_digital_inputs.txt

     Usage: ./O.linux-x86_64/central_node_engine_tst -f <file> -i <file>
            -f <file>   :  MPS database YAML file
       	    -i <file>   :  digital input test file
       	    -a <file>   :  analog input test file
       	    -v          :  verbose output
       	    -t          :  trace output
       	    -h          :  print this message
'''

Test Central Node
-----------------

Under the test directory the central_node_engine_server runs a test slow evaluation engine.
Instead of getting input status from and sending mitigation to the central node firmware this
test accepts input status from and sends mitigation to python test scripts.

It takes a yaml database as input:

'''
     central_node_engine_server -f yaml/mps_gun_config.yaml

     Usage: ./test/O.linux-x86_64/central_node_engine_server -f <file>
            -f <file>   :  MPS database YAML file
     	    -v          :  verbose output
	    -t          :  trace output
            -h          :  print this message
'''

The default socket port is 4356, but it if the environment variable CENTRAL_NODE_TEST_PORT is
defined the server uses that port number.

'''
	$ export CENTRAL_NODE_TEST_PORT 5566
'''

Important: in order to use the test port for receiving inputs the flag CXXFLAGS+=-DFW_ENABLED must 
be disabled/commented out in the defs.mak file.

Once the test server is running the central_node_test.py (from mps_database package) can be used to send 
input status, receive mitigation from the server and check against expected mitigation values.

By default the central node server does not evaluate fast rules, in the actual production system the
fast rules are evaluated by firmware. However, for testing the logic the following can be used to
enable fast evaluation by the software. This compile switch should be used for testing only:

'''
CXXFLAGS+= -DFAST_SW_EVALUATION
'''