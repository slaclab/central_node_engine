Pre-requirements
------------
Ensure you followed the initial steps of building the libraries from the TOP of directory within dev-rhel7 (to get the right compiler):
```
$ make install
```
It may take some time.

Tests
-----

The test subdirectory contains several *_tst.cc files that are stand alone testing programs:

As of (08-04-2023): central_node_database_tst is confirmed to work. The other tests may or may not work.

Usage: Reads the contents of YAML MPS Database, configure/check it and prints out the contents:

```     
     ./central_node_database_tst -f ../yaml/mps_config-cn1-2023-08-01-a.yaml -d

     Usage: ./O.linux-x86_64/central_node_database_tst [-f <file>] [-d]
       	    -f <file>   :  MPS database YAML file
       	    -d          :  dump YAML file to dump.txt file
       	    -t          :  trace output
       	    -h          :  print this message
```

If your current environment doesn't work, follow this:

```
$ cd src/test
$ make
# After compiled
$ ./central_node_database_tst -f ../yaml/mps_config-cn1-2023-08-01-a.yaml -d
```
If you run into errors with libraries or missing .so files, check what is missing:
```
$ ldd ./central_node_database_tst
```
Add the .so files in manually by coping them into central_node_engine/src/O.rhel7-x86_64

Then update your $LD_LIBRARY_PATH to link to those libraries. Ex:
```
$ export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/u/cd/<username>/mps/central_node_engine/src/O.rhel7-x86_64
```
* You may want to add it to .bashrc so you don't repeat this export each time you restart terminal.


Other
-----
I recommend using GDB or Valgrind for debugging
```
$ gdb --args ./central_node_database_tst -f ../yaml/mps_config-cn1-2023-08-02-a.yaml -d
$ valgrind --log-file="valgrind_dump.txt" ./central_node_database_tst -f ../yaml/mps_config-cn1-2023-08-01-a.yaml -d
```
If debugging with GDB, recommend adding [this](http://www.yolinux.com/TUTORIALS/src/dbinit_stl_views-1.03.txt) to your .gdbinit file in ~HOME dir, this will allow you to print STL containers like Maps.