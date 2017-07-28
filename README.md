Requirements
------------

central_node_engine uses the following libraries from $PACKAGE_TOP:

  yaml-cpp
  boost
  cpsw
  easylogging (for log messages - not working for LinuxRT yet see below how to disable)


Building
--------

$ make install
$ cd test
$ make


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

Test Central Node
-----------------

Under the test directory the central_node_engine_server runs a test slow evaluation engine.
Instead of getting input status from and sending mitigation to the central node firmware this
test accepts input status from and sends mitigation to python test scripts.

It takes a yaml database as input:

     central_node_engine_server -f yaml/mps_gun_config.yaml

     Usage: ./test/O.linux-x86_64/central_node_engine_server -f <file>
            -f <file>   :  MPS database YAML file
     	    -v          :  verbose output
	    -t          :  trace output
            -h          :  print this message

The default socket port is 4356, but it if the environment variable CENTRAL_NODE_TEST_PORT is
defined the server uses that port number.

	$ export CENTRAL_NODE_TEST_PORT 5566

Important: in order to use the test port for receiving inputs the flag CXXFLAGS+=-DFW_ENABLED must 
be disabled/commented out in the defs.mak file.

Once the test server is running the ./test/updateReceive.py script can be used to send 
input status, receive mitigation from the server and check against expected mitigation values.

      $ ./updateReceive.py -h
      usage: updateReceive.py [-h] [--host hostname] [--start [start_index]]
                              [--size [size]] [--port [size]] [--debug]
                              [--repeat [number]]
                              input_name mitigation_name
      Send link node update to central_node_engine server

      positional arguments:
        input_name            input file
	mitigation_name       expected mitigation file

      optional arguments:
        -h, --help            show this help message and exit
	--host hostname       Central node hostname
	--start [start_index] start index (default=1)
	--size [size]         number of input files (default=1)
 	--port [size]         server port (default=4356)
	--debug               enable debug output
	--repeat [number]     repeat a finite number of times (default=0 -> forever)

Example:

	$ ./updateReceive.py inputs/input-state inputs/mitigation --start 1 --size 5 --repeat 2 --port 5566
	Sending 5 updates to central node engine at lcls-dev3:5566. Repeating 2 times.
	Update cycle #1
	Update #1 sent. (file=inputs/input-state-1.txt)
	Update #2 sent. (file=inputs/input-state-2.txt)
	Update #3 sent. (file=inputs/input-state-3.txt)
	Update #4 sent. (file=inputs/input-state-4.txt)
	Update #5 sent. (file=inputs/input-state-5.txt)
	Update cycle #2
	Update #1 sent. (file=inputs/input-state-1.txt)
	Update #2 sent. (file=inputs/input-state-2.txt)
	Update #3 sent. (file=inputs/input-state-3.txt)
	Update #4 sent. (file=inputs/input-state-4.txt)
	Update #5 sent. (file=inputs/input-state-5.txt)

The input-state-X.txt file is a text file, where each line has the 'wasLow'/'wasHigh' state for
each digital/analog input for an application card.

The mitigation-X.txt is a text file contaning a single line with expected power classes for
the mitigation devices.

