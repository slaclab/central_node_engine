# RELEASE_NOTES

Release notes for the LCLS-II MPS central node engine.

## Releases:

  * Add ignore conditions for digital devices to go with those in analog devices
    * Change analogDeviceId in DbIgnoreCondition to deviceId
    * Add digitalDevice to DbIgnoreCondition
    * Connect either an analog or digital device to IgnoreCondition in central_node_database.cc
    * Evaluate digitalDevice ignore conditions in central_node_engine.cc
    * In central_node_engine.cc::mitigate(), add bit to check if a fault should be ignored:
      * Link a fault to a device (ignore condition bool is currently stored with a device) through
        the fault inputs tied to the fault.

* __central_node_engine-R2-4-0__:
  * Update fault reporting to PVs so that the fault value is reported
  * tie latched faults to latched value of input value

* __central_node_engine-R2-3-0__:
  * Change logFault fault to send to history server:
      Fault ID
      Old Fault Value
      New Fault Value
      Allowed Class ID - will be 0 if no allowed class (no fault)
  * Stop sending logMitigation
  * Fixed bug with forceBeamClass in setAllowedBeamClass() where the forceBeamClass would
    overwrite whatever the actual logic was trying to set.  Now final beam class is the
    lowest of the force and the logic output.

* __central_node_engine-R2-2-1__:
  * Correct destination mask behavior for SW logic to allow multiple bits in mask per destination
      allow mapping one to many

# __central_node_engine-R2-2-0__:
  * Add auto-unlatch flag for digital inputs.  This will cause latched 
    value to be set to current value if True

* __central_node_engine-R2-1-1__:
  * Write the app timeout enable bits to FW all at once when they are
    set to true by default. When they are set using the API call
    setAppTimeoutEnable(), by default they are write to FW as well.
  * Only set the app timeout enable bit to true when loading the
    configuration YAML file the first time.

* __central_node_engine-R2-1-0__:
  * Add new method to read the application timeout enable bits:
    `getAppTimeoutEnable()`.
  * Add a call to `writeAppTimeoutMask()` in `setAppTimeoutEnable()` in order
    to write to FW the configuration bits when the timeout bits are changed.

* __central_node_engine-R2-0-0__:
  * Increases the number of supported applications from `8` to `1024`.
  * Adjust RT priorities of all the evaluation threads as described in this
    table (note that the NIC kernel thread priorities are not set as part of
    this repository, but instead on the CPU startup script):
    | Thread               | Priority |
    |----------------------|----------|
    | NIC (kernel)         |    80    |
    | CPSW: UDP            |    81    |
    | CPSW: RSSI           |    82    |
    | CPSW: Depack         |    83    |
    | CPSW: TDESTMux       |    84    |
    | Engine: FwReader     |    85    |
    | Engine: InputUpdates |    86    |
    | Engine: HeartBeat    |    87    |
    | Engine: MitWriter    |    87    |
    | Engine: EngineThread |    88    |
  * Add a thread-safe `Queue` class.
  * Migrate the update input and the mitigation shared buffers to a `Queue`
    object. This will give the system more elasticity to absorb delays between
    threads, instead of blocking the whole pipeline.
  * heartbeat: modify the `NonBlocking` policy class to keep a request counter,
    instead of a flag, in order to avoid blocking the requesting thread in case
    the previous heartbeat takes longer than `2.7ms` to write the FW register.
  * Bug fix on `central_node_database`: `inputProcessed()` should lock the
    `mutex` and notify using the `condition_variable`.
  * Add name to the `fwPCChangeThread` thread.
  * Fix format on some source files to improve readability.

* __central_node_engine-R1-8-0__:
  * Implement power class change asynchronous notification message
    handler.
  * Fix bugs the HeartBeat class, which was generating long periods
    between heartbeats.
  * Fix bugs and improve the CN engine.

* __central_node_engine-R1-7-0__:
  * Update buildroot to version 2019.08.
  * Update CPSW to version R4.4.1. This implied update boost to version
    1.64.0 and yaml-cpp to version yaml-cpp-0.5.3_boost-1.64.0.
  * Fix all compilation warnings.
  * Merge changes from the EIC branch.

* __central_node_engine-R1-6-0-eic__:
  * Changed logic to allow AOM
  * Bug fixed on the FW configuration with AOM bypassed - wrong
    bits were being changed

* __central_node_engine-R1-5-0-eic__:
  * Change to support AOM enable while mechanical laser shutter
    is closed (works with mps_configuration EIC branch and
    central_node_ioc branches)

* __central_node_engine-R1-4-1__:
  * Change to how the charge integration time is written to FW.
    The value is broken up into bits 0:9 and 16:25.

* __central_node_engine-R1-4-0__:
  * Compatible with mps_database-R1-1-0, which has a change in the
    crates.number to crates.crate_id. This required code changes
    in this library.

* __central_node_engine-R1-3-3__:
  * Implemented bypass for fast digital devices (they must have
    a single device input). The firmware configuration is reloaded
    when a fast digital input bypass is created or expires.

* __central_node_engine-R1-3-2__:
  * Addition of functionality to allow forcing a power class to
    o beam destination (e.g. allow ops to shut off gun by setting
    a PV).
  * Addition of missing firmware call to run central node stand-alone,
    without connecting to central node card.

* __central_node_engine-R1-3-1__:
  * Change introduced in R1-2-5 stops MPS whenever the update thread
    is stopped/started. Modified so it doesn't start MPS only after
    loading a configuration the first time.

* __central_node_engine-R1-3-0__:
  * Addition of Firmware::beamFaultClear(), for clearing
    faults caused by power class or charge violation

* __central_node_engine-R1-2-5__:
  * After config is loaded MPS is not enabled by default, user
    must enable it on the EDM panel

* __central_node_engine-R1-2-4__:
  * Minor fix on the beam charge table readback - changed from 16 to 32 bits

* __central_node_engine-R1-2-3__:
  * Restored loading beam charge table using database values
