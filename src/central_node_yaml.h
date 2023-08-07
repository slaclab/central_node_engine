/**
 * central_node_yaml.h
 *
 * Template specializations to convert MPS YAML database into
 * C++ objects. The C++ classes are described in the file
 * central_node_database.h.
 */

#ifndef CENTRAL_NODE_YAML_H
#define CENTRAL_NODE_YAML_H

#include <central_node_database_defs.h>
#include <central_node_database_tables.h>
#include <yaml-cpp/yaml.h>


namespace YAML {

  /**
   * Crate:
   * - id: '1'
   *   num_slots: '6'
   *   number: '1'
   *   shelf_number: '1'
   */
  template<>
    struct convert<DbCrateMapPtr> {
    static bool decode(const Node &node, DbCrateMapPtr &rhs) {
      DbCrateMap *crates = new DbCrateMap();
      std::stringstream errorStream;
      std::string field;
      rhs = DbCrateMapPtr(crates);

      for (YAML::Node::const_iterator it = node["Crate"].begin();
	   it != node["Crate"].end(); ++it) {
	DbCrate *crate = new DbCrate();

	try {
	  field = "id";
	  crate->id = (*it)[field].as<unsigned int>();

	  field = "crate_id";
	  crate->crate_id = (*it)[field].as<unsigned int>();

	  field = "num_slots";
	  crate->numSlots = (*it)[field].as<unsigned int>();

	  field = "shelf_number";
	  crate->shelfNumber = (*it)[field].as<unsigned int>();
	} catch(YAML::InvalidNode &e) {
	  errorStream << "ERROR: Failed to find field " << field << " for Crate.";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<unsigned int> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for Crate (expected unsigned int).";
	  throw(DbException(errorStream.str()));
	}

	rhs->insert(std::pair<int, DbCratePtr>(crate->id, DbCratePtr(crate)));
      }

      return true;
    }
  };

  /**
   * Info:
   * - analog_channel_count: '3'
   *   analog_channel_size: '8'
   *   digital_channel_count: '4'
   *   digital_channel_size: '1'
   *   id: '1'
   *   name: Mixed Mode Link Node
   *   number: '0'
   */
  template<>
    struct convert<DbInfoMapPtr> {
    static bool decode(const Node &node, DbInfoMapPtr &rhs) {
      DbInfoMap *appTypes = new DbInfoMap();
      std::stringstream errorStream;
      std::string field;
      rhs = DbInfoMapPtr(appTypes);

      for (YAML::Node::const_iterator it = node["DatabaseInfo"].begin();
	   it != node["DatabaseInfo"].end(); ++it) {
	DbInfo *dbInfo = new DbInfo();

	try {
	  field = "source";
	  dbInfo->source = (*it)[field].as<std::string>();

	  field = "user";
	  dbInfo->user = (*it)[field].as<std::string>();

	  field = "date";
	  dbInfo->date = (*it)[field].as<std::string>();

	  field = "md5sum";
	  dbInfo->md5sum = (*it)[field].as<std::string>();
	} catch(YAML::InvalidNode &e) {
	  errorStream << "ERROR: Failed to find field " << field << " for DatabaseInfo.";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<std::string> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for DatabaseInfo (expected string).";
	  throw(DbException(errorStream.str()));
	}

	rhs->insert(std::pair<int, DbInfoPtr>(0, DbInfoPtr(dbInfo)));
      }

      return true;
    }
  };

	/**
 	* ApplicationType:
	*   analog_channel_count: 3
	*   digital_channel_count: 4
	*	software_channel_count: 32
	*   id: 1
	*   name: 'MPS Analog'
	*   number: 3
	*/
  template<>
    struct convert<DbApplicationTypeMapPtr> {
		static bool decode(const Node &node, DbApplicationTypeMapPtr &rhs) {
			DbApplicationTypeMap *appTypes = new DbApplicationTypeMap();
			std::stringstream errorStream;
			std::string field;
			rhs = DbApplicationTypeMapPtr(appTypes);

			for (YAML::Node::const_iterator it = node["ApplicationType"].begin();
				it != node["ApplicationType"].end(); ++it) {
				DbApplicationType *appType = new DbApplicationType();

				try {
					field = "id";
					appType->id = (*it)[field].as<unsigned int>();

					field = "num_integrators";
					appType->num_integrators = (*it)[field].as<unsigned int>();

					field = "analog_channel_count";
					appType->analogChannelCount = (*it)[field].as<unsigned int>();

					field = "digital_channel_count";
					appType->digitalChannelCount = (*it)[field].as<unsigned int>();

					field = "software_channel_count";
					appType->softwareChannelCount = (*it)[field].as<unsigned int>();

					field = "name";
					appType->description = (*it)[field].as<std::string>();
				} catch(YAML::InvalidNode &e) {
					errorStream << "ERROR: Failed to find field " << field << " for ApplicationType.";
					throw(DbException(errorStream.str()));
				} catch(YAML::TypedBadConversion<unsigned int> &e) {
					errorStream << "ERROR: Failed to convert contents of field " << field << " for ApplicationType (expected unsigned int).";
					throw(DbException(errorStream.str()));
				} catch(YAML::TypedBadConversion<std::string> &e) {
					errorStream << "ERROR: Failed to convert contents of field " << field << " for ApplicationType (expected string).";
					throw(DbException(errorStream.str()));
				}

				rhs->insert(std::pair<int, DbApplicationTypePtr>(appType->id,
										DbApplicationTypePtr(appType)));
			}

			return true;
		}
  	};

  /**
   * ApplicationCard:
   * - crate_id: '1'
   *   id: '1'
   *   number: '1'
   *   slot_number: '2'
   *   type_id: '1'
   *   global_id: 0
   *   name: "EIC Digital"
   *   description: "EIC Digital Status"
   */
  template<>
    struct convert<DbApplicationCardMapPtr> {
    static bool decode(const Node &node, DbApplicationCardMapPtr &rhs) {
      DbApplicationCardMap *appCards = new DbApplicationCardMap();
      std::stringstream errorStream;
      std::string field;
      rhs = DbApplicationCardMapPtr(appCards);

      for (YAML::Node::const_iterator it = node["ApplicationCard"].begin();
	   it != node["ApplicationCard"].end(); ++it) {
	DbApplicationCard *appCard = new DbApplicationCard();

	try {
	  field = "id";
	  appCard->id = (*it)[field].as<unsigned int>();

	  field = "number";
	  appCard->number = (*it)[field].as<unsigned int>();

	  field = "crate_id";
	  appCard->crateId = (*it)[field].as<unsigned int>();

	  field = "slot_number";
	  appCard->slotNumber = (*it)[field].as<unsigned int>();

	  field = "type_id";
	  appCard->applicationTypeId = (*it)[field].as<unsigned int>();

	  field = "global_id";
	  appCard->globalId = (*it)[field].as<unsigned int>();

	  field = "name";
	  appCard->name = (*it)[field].as<std::string>();

	  field = "description";
	  appCard->description = (*it)[field].as<std::string>();
	} catch(YAML::InvalidNode &e) {
	  errorStream << "ERROR: Failed to find field " << field << " for ApplicationCard.";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<unsigned int> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for ApplicationCard (expected unsigned int).";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<std::string> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for ApplicationCard (expected string).";
	  throw(DbException(errorStream.str()));
	}

	rhs->insert(std::pair<int, DbApplicationCardPtr>(appCard->id,
							 DbApplicationCardPtr(appCard)));
      }

      return true;
    }
  };

  /**
   * DigitalChannel:
   * - card_id: '1'
   *   id: '1'
   *   number: '0'
   *
   * AnalogChannel:
   * - card_id: '1'
   *   id: '1'
   *   number: '0'
   */
  template<>
    struct convert<DbChannelMapPtr> {
    static bool decode(const Node &node, DbChannelMapPtr &rhs) {
      std::string key = "DigitalChannel";

      try {
	node[key].size();
      } catch (InvalidNode &e) {
	key = "AnalogChannel";
      }

      DbChannelMap *channels = new DbChannelMap();
      std::stringstream errorStream;
      std::string field;
      rhs = DbChannelMapPtr(channels);

      for (YAML::Node::const_iterator it = node[key].begin();
	   it != node[key].end(); ++it) {
	DbChannel *channel = new DbChannel();

	try {
	  field = "id";
	  channel->id = (*it)[field].as<unsigned int>();

	  field = "name";
	  channel->name = (*it)[field].as<std::string>();

	  field = "number";
	  channel->number = (*it)[field].as<unsigned int>();

	  field = "card_id";
	  channel->cardId = (*it)[field].as<unsigned int>();
	} catch(YAML::InvalidNode &e) {
	  errorStream << "ERROR: Failed to find field " << field << " for " << key << ".";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<unsigned int> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for " << key << " (expected unsigned int).";
	  throw(DbException(errorStream.str()));
	}

	rhs->insert(std::pair<int, DbChannelPtr>(channel->id, DbChannelPtr(channel)));
      }

      return true;
    }
  };

  /**
   * DeviceType:
   * - id: '1'
   *   name: Insertion Device
   */
  template<>
    struct convert<DbDeviceTypeMapPtr> {
    static bool decode(const Node &node, DbDeviceTypeMapPtr &rhs) {
      DbDeviceTypeMap *deviceTypes = new DbDeviceTypeMap();
      std::stringstream errorStream;
      std::string field;
      rhs = DbDeviceTypeMapPtr(deviceTypes);

      for (YAML::Node::const_iterator it = node["DeviceType"].begin();
	   it != node["DeviceType"].end(); ++it) {
	DbDeviceType *deviceType = new DbDeviceType();

	try {
	  field = "id";
	  deviceType->id = (*it)[field].as<unsigned int>();

	  field = "name";
	  deviceType->name = (*it)[field].as<std::string>();

	  field = "num_integrators";
	  deviceType->numIntegrators = (*it)[field].as<unsigned int>();
	} catch(YAML::InvalidNode &e) {
	  errorStream << "ERROR: Failed to find field " << field << " for DeviceType.";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<unsigned int> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for DeviceType (expected unsigned int).";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<std::string> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for DeviceType (expected string).";
	  throw(DbException(errorStream.str()));
	}

	rhs->insert(std::pair<int, DbDeviceTypePtr>(deviceType->id,
						    DbDeviceTypePtr(deviceType)));
      }

      return true;
    }
  };

  /**
   * DeviceState:
   * - device_type_id: '1'
   *   id: '1'
   *   name: Out
   *   value: '1'
   */
  template<>
    struct convert<DbDeviceStateMapPtr> {
    static bool decode(const Node &node, DbDeviceStateMapPtr &rhs) {
      DbDeviceStateMap *deviceStates = new DbDeviceStateMap();
      std::stringstream errorStream;
      std::string field;
      rhs = DbDeviceStateMapPtr(deviceStates);

      for (YAML::Node::const_iterator it = node["DeviceState"].begin();
	   it != node["DeviceState"].end(); ++it) {
	DbDeviceState *	deviceState = new DbDeviceState();

	try {
	  field = "id";
	  deviceState->id = (*it)[field].as<unsigned int>();

	  field = "value";
	  deviceState->value = (*it)[field].as<unsigned int>();

	  field = "mask";
	  deviceState->mask = (*it)[field].as<uint32_t>();

	  field = "device_type_id";
	  deviceState->deviceTypeId = (*it)[field].as<unsigned int>();

	  field = "name";
	  deviceState->name = (*it)[field].as<std::string>();
 	} catch(YAML::InvalidNode &e) {
	  errorStream << "ERROR: Failed to find field " << field << " for DeviceState.";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<unsigned int> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for DeviceState (expected unsigned int).";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<std::string> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for DeviceState (expected string).";
	  throw(DbException(errorStream.str()));
	}

	rhs->insert(std::pair<int, DbDeviceStatePtr>(deviceState->id,
						     DbDeviceStatePtr(deviceState)));
      }

      return true;
    }
  };

  /**
   * DigitalDevice:
   * - device_type_id: '1'
   *   id: '1'
   */
  template<>
    struct convert<DbDigitalDeviceMapPtr> {
    static bool decode(const Node &node, DbDigitalDeviceMapPtr &rhs) {
      DbDigitalDeviceMap *digitalDevices = new DbDigitalDeviceMap();
      std::stringstream errorStream;
      std::string field;
      rhs = DbDigitalDeviceMapPtr(digitalDevices);

      for (YAML::Node::const_iterator it = node["DigitalDevice"].begin();
	   it != node["DigitalDevice"].end(); ++it) {
	DbDigitalDevice *digitalDevice = new DbDigitalDevice();

	try {
	  field = "id";
	  digitalDevice->id = (*it)[field].as<unsigned int>();

	  field = "device_type_id";
	  digitalDevice->deviceTypeId = (*it)[field].as<unsigned int>();

	  field = "name";
	  digitalDevice->name = (*it)[field].as<std::string>();

	  field = "description";
	  digitalDevice->description = (*it)[field].as<std::string>();

	  field = "evaluation";
	  digitalDevice->evaluation = (*it)[field].as<unsigned int>();

	  field = "card_id";
	  digitalDevice->cardId = (*it)[field].as<unsigned int>();

	  digitalDevice->value = 0;
  	} catch(YAML::InvalidNode &e) {
	  errorStream << "ERROR: Failed to find field " << field << " for DigitalDevice.";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<unsigned int> &e) {
	  // There may be devices that do not have cards assigned, but have secondary measurement
	  // devices that read some of its properties.
	  if (field == "card_id") {
	    digitalDevice->cardId = NO_CARD_ID;
	  }
	  else {
	    errorStream << "ERROR: Failed to convert contents of field " << field << " for DigitalDevice (expected unsigned int).";
	    throw(DbException(errorStream.str()));
	  }
	} catch(YAML::TypedBadConversion<std::string> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for DigitalDevice (expected string).";
	  throw(DbException(errorStream.str()));
	}

	rhs->insert(std::pair<int, DbDigitalDevicePtr>(digitalDevice->id,
						       DbDigitalDevicePtr(digitalDevice)));
      }

      return true;
    }
  };

  /**
   * DeviceInput:
   * - bit_position: '0'
   *   fault_value: '0'
   *   channel_id: '1'
   *   digital_device_id: '1'
   *   auto_reset: True
   *   id: '1'
   */
  template<>
    struct convert<DbDeviceInputMapPtr> {
    static bool decode(const Node &node, DbDeviceInputMapPtr &rhs) {
      DbDeviceInputMap *deviceInputs = new DbDeviceInputMap();
      std::stringstream errorStream;
      std::string field;
      rhs = DbDeviceInputMapPtr(deviceInputs);

      for (YAML::Node::const_iterator it = node["DeviceInput"].begin();
	   it != node["DeviceInput"].end(); ++it) {
	DbDeviceInput *deviceInput = new DbDeviceInput();

	try {
	  field = "id";
	  deviceInput->id = (*it)[field].as<unsigned int>();

	  field = "bit_position";
	  deviceInput->bitPosition = (*it)[field].as<unsigned int>();

	  field = "fault_value";
	  deviceInput->faultValue = (*it)[field].as<unsigned int>();

	  field = "digital_device_id";
	  deviceInput->digitalDeviceId = (*it)[field].as<unsigned int>();

	  field = "channel_id";
	  deviceInput->channelId = (*it)[field].as<unsigned int>();

	  field = "auto_reset";
	  deviceInput->autoReset = (*it)[field].as<unsigned int>();

	  deviceInput->value = 0;
 	} catch(YAML::InvalidNode &e) {
	  errorStream << "ERROR: Failed to find field " << field << " for DeviceInput.";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<unsigned int> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for DeviceInput (expected unsigned int).";
	  throw(DbException(errorStream.str()));
	}

	rhs->insert(std::pair<int, DbDeviceInputPtr>(deviceInput->id,
						     DbDeviceInputPtr(deviceInput)));
      }

      return true;
    }
  };

  /**
   * Condition:
   * - id: 1
   *   name: "YAG01_IN"
   *   description: "YAG01 screen IN"
   *   value: 1
   */
  template<>
    struct convert<DbConditionMapPtr> {
    static bool decode(const Node &node, DbConditionMapPtr &rhs) {
      DbConditionMap *conditions = new DbConditionMap();
      std::stringstream errorStream;
      std::string field;
      rhs = DbConditionMapPtr(conditions);

      for (YAML::Node::const_iterator it = node["Condition"].begin();
	   it != node["Condition"].end(); ++it) {
	DbCondition *condition = new DbCondition();

	try {
	  field = "id";
	  condition->id = (*it)[field].as<unsigned int>();

	  field = "name";
	  condition->name = (*it)[field].as<std::string>();

	  field = "description";
	  condition->description = (*it)[field].as<std::string>();

	  field = "value";
	  condition->mask = (*it)[field].as<unsigned int>();
  	} catch(YAML::InvalidNode &e) {
	  errorStream << "ERROR: Failed to find field " << field << " for Condition.";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<unsigned int> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for Condition (expected unsigned int).";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<std::string> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for Condition (expected string).";
	  throw(DbException(errorStream.str()));
	}

	rhs->insert(std::pair<int, DbConditionPtr>(condition->id, DbConditionPtr(condition)));
      }

      return true;
    }
  };

  /**
   * IgnoreCondition:
   * - id: 1
   *   condition_id: 1
   *   fault_state_id: 14
   */
  template<>
    struct convert<DbIgnoreConditionMapPtr> {
    static bool decode(const Node &node, DbIgnoreConditionMapPtr &rhs) {
      DbIgnoreConditionMap *ignoreConditions = new DbIgnoreConditionMap();
      std::stringstream errorStream;
      std::string field;
      rhs = DbIgnoreConditionMapPtr(ignoreConditions);

      for (YAML::Node::const_iterator it = node["IgnoreCondition"].begin();
	   it != node["IgnoreCondition"].end(); ++it) {
	DbIgnoreCondition *ignoreCondition = new DbIgnoreCondition();

	ignoreCondition->faultStateId = DbIgnoreCondition::INVALID_ID;
	ignoreCondition->deviceId = DbIgnoreCondition::INVALID_ID;

	try {
	  field = "id";
	  ignoreCondition->id = (*it)[field].as<unsigned int>();

	  field = "condition_id";
	  ignoreCondition->conditionId = (*it)[field].as<unsigned int>();
 	} catch(YAML::InvalidNode &e) {
	  errorStream << "ERROR: Failed to find field " << field << " for IgnoreCondition.";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<unsigned int> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for IgnoreCondition (expected unsigned int).";
	  throw(DbException(errorStream.str()));
	}

	bool found = false;
	try {
	  field = "fault_state_id";
	  ignoreCondition->faultStateId = (*it)[field].as<unsigned int>();
	  found = true;
 	} catch(YAML::InvalidNode &e) {
	  errorStream << "ERROR: Failed to find field " << field << " for IgnoreCondition.";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<unsigned int> &e) {
	  // ... just continue, this exception is fine as long an there is an analogDeviceId defined next
	}

	try {
	  field = "device_id";
	  ignoreCondition->deviceId = (*it)[field].as<unsigned int>();
	  found = true;
	} catch(YAML::InvalidNode &e) {
	  errorStream << "ERROR: Failed to find field " << field << " for IgnoreCondition.";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<unsigned int> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for IgnoreCondition (expected unsigned int).";
	  throw(DbException(errorStream.str()));
	}

	if (!found) {
	  errorStream << "ERROR: IgnoreCondition without faultState or analogDevice defined";
	  throw(DbException(errorStream.str()));
	}

	rhs->insert(std::pair<int, DbIgnoreConditionPtr>(ignoreCondition->id, DbIgnoreConditionPtr(ignoreCondition)));
      }

      return true;
    }
  };

  /**
   * ConditionInput:
   * - id: 1
   *   bit_position: 0
   *   fault_state_id: 1
   *   condition_id: 1
   */
  template<>
    struct convert<DbConditionInputMapPtr> {
    static bool decode(const Node &node, DbConditionInputMapPtr &rhs) {
      DbConditionInputMap *conditionInputs = new DbConditionInputMap();
      std::stringstream errorStream;
      std::string field;
      rhs = DbConditionInputMapPtr(conditionInputs);

      for (YAML::Node::const_iterator it = node["ConditionInput"].begin();
	   it != node["ConditionInput"].end(); ++it) {
	DbConditionInput *conditionInput = new DbConditionInput();

	try {
	  field = "id";
	  conditionInput->id = (*it)[field].as<unsigned int>();

	  field = "bit_position";
	  conditionInput->bitPosition = (*it)[field].as<unsigned int>();

	  field = "fault_state_id";
	  conditionInput->faultStateId = (*it)[field].as<unsigned int>();

	  field = "condition_id";
	  conditionInput->conditionId = (*it)[field].as<unsigned int>();
 	} catch(YAML::InvalidNode &e) {
	  errorStream << "ERROR: Failed to find field " << field << " for ConditionInput.";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<unsigned int> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for ConditionInput (expected unsigned int).";
	  throw(DbException(errorStream.str()));
	}

	rhs->insert(std::pair<int, DbConditionInputPtr>(conditionInput->id, DbConditionInputPtr(conditionInput)));
      }

      return true;
    }
  };

  /**
   * Fault:
   * - description: None
   *   id: '1'
   *   name: OTR Fault
   */
  template<>
    struct convert<DbFaultMapPtr> {
    static bool decode(const Node &node, DbFaultMapPtr &rhs) {
      DbFaultMap *faults = new DbFaultMap();
      std::stringstream errorStream;
      std::string field;
      rhs = DbFaultMapPtr(faults);

      for (YAML::Node::const_iterator it = node["Fault"].begin();
	   it != node["Fault"].end(); ++it) {
	DbFault *fault = new DbFault();

	try {
	  field = "id";
	  fault->id = (*it)[field].as<unsigned int>();

	  field = "name";
	  fault->name = (*it)[field].as<std::string>();

	  field = "description";
	  fault->description = (*it)[field].as<std::string>();
	  fault->value = 0;
  	} catch(YAML::InvalidNode &e) {
	  errorStream << "ERROR: Failed to find field " << field << " for Fault.";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<unsigned int> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for Fault (expected unsigned int).";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<std::string> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for Fault (expected string).";
	  throw(DbException(errorStream.str()));
	}

	rhs->insert(std::pair<int, DbFaultPtr>(fault->id, DbFaultPtr(fault)));
      }

      return true;
    }
  };

  /**
   * FaultInput:
   * - bit_position: '0'
   *   device_id: '1'
   *   fault_id: '1'
   *   id: '1'
   */
  template<>
    struct convert<DbFaultInputMapPtr> {
    static bool decode(const Node &node, DbFaultInputMapPtr &rhs) {
      DbFaultInputMap *faultInputs = new DbFaultInputMap();
      std::stringstream errorStream;
      std::string field;
      rhs = DbFaultInputMapPtr(faultInputs);

      for (YAML::Node::const_iterator it = node["FaultInput"].begin();
	   it != node["FaultInput"].end(); ++it) {
	DbFaultInput *faultInput = new DbFaultInput();

	try {
	  field = "id";
	  faultInput->id = (*it)[field].as<unsigned int>();

	  field = "bit_position";
	  faultInput->bitPosition = (*it)[field].as<unsigned int>();

	  field = "device_id";
	  faultInput->deviceId = (*it)[field].as<unsigned int>();

	  field = "fault_id";
	  faultInput->faultId = (*it)[field].as<unsigned int>();

	  faultInput->value = 0;
  	} catch(YAML::InvalidNode &e) {
	  errorStream << "ERROR: Failed to find field " << field << " for FaultInput.";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<unsigned int> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for FaultInput (expected unsigned int).";
	  throw(DbException(errorStream.str()));
	}

	rhs->insert(std::pair<int, DbFaultInputPtr>(faultInput->id,
						    DbFaultInputPtr(faultInput)));
      }

      return true;
    }
  };

  /**
   * FaultState:
   * - fault_id: '1'
   *   id: '1'
   *   name: Out
   *   value: '1'
   */
  template<>
    struct convert<DbFaultStateMapPtr> {
    static bool decode(const Node &node, DbFaultStateMapPtr &rhs) {
      DbFaultStateMap *faultStates = new DbFaultStateMap();
      std::stringstream errorStream;
      std::string field;
      rhs = DbFaultStateMapPtr(faultStates);

      for (YAML::Node::const_iterator it = node["FaultState"].begin();
	   it != node["FaultState"].end(); ++it) {
	DbFaultState *faultState = new DbFaultState();

	try {
	  field = "id";
	  faultState->id = (*it)[field].as<unsigned int>();

	  field = "fault_id";
	  faultState->faultId = (*it)[field].as<unsigned int>();

	  field = "device_state_id";
	  faultState->deviceStateId = (*it)[field].as<unsigned int>();

	  field = "default";
	  faultState->defaultState = (*it)[field].as<bool>();
  	} catch(YAML::InvalidNode &e) {
	  errorStream << "ERROR: Failed to find field " << field << " for FaultState.";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<unsigned int> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for FaultState (expected unsigned int).";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<bool> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for FaultState (expected bool).";
	  throw(DbException(errorStream.str()));
	}

	rhs->insert(std::pair<int, DbFaultStatePtr>(faultState->id,
						    DbFaultStatePtr(faultState)));
      }

      return true;
    }
  };

  /**
   * AnalogDevice:
   * - analog_device_type_id: '1'
   *   channel_id: '1'
   *   id: '3'
   */
  template<>
    struct convert<DbAnalogDeviceMapPtr> {
    static bool decode(const Node &node, DbAnalogDeviceMapPtr &rhs) {
      DbAnalogDeviceMap *analogDevices = new DbAnalogDeviceMap();
      std::stringstream errorStream;
      std::string field;
      rhs = DbAnalogDeviceMapPtr(analogDevices);

      for (YAML::Node::const_iterator it = node["AnalogDevice"].begin();
	   it != node["AnalogDevice"].end(); ++it) {
	DbAnalogDevice *analogDevice = new DbAnalogDevice();

	try {
	  field = "id";
	  analogDevice->id = (*it)[field].as<unsigned int>();

	  field = "device_type_id";
	  analogDevice->deviceTypeId = (*it)[field].as<unsigned int>();

	  field = "channel_id";
	  analogDevice->channelId = (*it)[field].as<unsigned int>();

	  field = "name";
	  analogDevice->name = (*it)[field].as<std::string>();

	  field = "description";
	  analogDevice->description = (*it)[field].as<std::string>();

	  field = "evaluation";
	  analogDevice->evaluation = (*it)[field].as<unsigned int>();

	  field = "card_id";
	  analogDevice->cardId = (*it)[field].as<unsigned int>();
	  analogDevice->value = 0;
	  analogDevice->bypassMask = 0xFFFFFFFF;
  	} catch(YAML::InvalidNode &e) {
	  errorStream << "ERROR: Failed to find field " << field << " for AnalogDevice.";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<unsigned int> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for AnalogDevice (expected unsigned int).";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<std::string> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for AnalogDevice (expected string).";
	  throw(DbException(errorStream.str()));
	}

	rhs->insert(std::pair<int, DbAnalogDevicePtr>(analogDevice->id,
						      DbAnalogDevicePtr(analogDevice)));
      }

      return true;
    }
  };

	/**
	 * BeamDestination:
	 *   id: 1
	 *   name: 'LASER'
	 *   destination_mask: 1
	 *   display_order: 7
	 */
  template<>
    struct convert<DbBeamDestinationMapPtr> {
		static bool decode(const Node &node, DbBeamDestinationMapPtr &rhs) {
			DbBeamDestinationMap *beamDestinations = new DbBeamDestinationMap();
			std::stringstream errorStream;
			std::string field;
			rhs = DbBeamDestinationMapPtr(beamDestinations);

			for (YAML::Node::const_iterator it = node["BeamDestination"].begin();
				it != node["BeamDestination"].end(); ++it) {
				DbBeamDestination *beamDestination = new DbBeamDestination();

				try {
					field = "id";
					beamDestination->id = (*it)[field].as<unsigned int>();

					field = "name";
					beamDestination->name = (*it)[field].as<std::string>();

					field = "mask";
					beamDestination->destinationMask = (*it)[field].as<short>();

					field = "display_order";
					beamDestination->displayOrder = (*it)[field].as<short>();

					// Find the device position in the softwareMitigationBuffer (32-bit array)
					// Each mitigation device defines has a 4-bit power class, the
					// position tells in which byte it falls based on the
					// destination mask

					// Destination mask
					// [16 15 14 13 12 11 10 09][08 07 06 05 04 03 02 01]
					beamDestination->softwareMitigationBufferIndex = 1; // when FW is fixed this goes back to index 0
					beamDestination->bitShift = 0;
					uint16_t mask = beamDestination->destinationMask;
					beamDestination->buffer0DestinationMask = 0;
					beamDestination->buffer1DestinationMask = 0;
					if ((beamDestination->destinationMask & 0x1) > 0) {
						beamDestination->buffer1DestinationMask |= 0x0000000F;
					}
					if ((beamDestination->destinationMask & 0x2) > 0) {
						beamDestination->buffer1DestinationMask |= 0x000000F0;
					}
					if ((beamDestination->destinationMask & 0x4) > 0) {
						beamDestination->buffer1DestinationMask |= 0x00000F00;
					}
					if ((beamDestination->destinationMask & 0x8) > 0) {
						beamDestination->buffer1DestinationMask |= 0x0000F000;
					}
					if ((beamDestination->destinationMask & 0x10) > 0) {
						beamDestination->buffer1DestinationMask |= 0x000F0000;
					}
					if ((beamDestination->destinationMask & 0x20) > 0) {
						beamDestination->buffer1DestinationMask |= 0x00F00000;
					}
					if ((beamDestination->destinationMask & 0x40) > 0) {
						beamDestination->buffer1DestinationMask |= 0x0F000000;
					}
					if ((beamDestination->destinationMask & 0x80) > 0) {
						beamDestination->buffer1DestinationMask |= 0xF0000000;
					}
					if ((beamDestination->destinationMask & 0x100) > 0) {
						beamDestination->buffer0DestinationMask |= 0x0000000F;
					}
					if ((beamDestination->destinationMask & 0x200) > 0) {
						beamDestination->buffer0DestinationMask |= 0x000000F0;
					}
					if ((beamDestination->destinationMask & 0x400) > 0) {
						beamDestination->buffer0DestinationMask |= 0x00000F00;
					}
					if ((beamDestination->destinationMask & 0x800) > 0) {
						beamDestination->buffer0DestinationMask |= 0x0000F000;
					}
					if ((beamDestination->destinationMask & 0x1000) > 0) {
						beamDestination->buffer0DestinationMask |= 0x000F0000;
					}
					if ((beamDestination->destinationMask & 0x2000) > 0) {
						beamDestination->buffer0DestinationMask |= 0x00F00000;
					}
					if ((beamDestination->destinationMask & 0x4000) > 0) {
						beamDestination->buffer0DestinationMask |= 0x0F000000;
					}
					if ((beamDestination->destinationMask & 0x8000) > 0) {
						beamDestination->buffer0DestinationMask |= 0xF0000000;
					}
					if ((mask & 0xFF00) > 0) {
						// beamDestination->softwareMitigationBufferIndex = 1; // if destination bit set from 8 to 15, use second mitigation position
						beamDestination->softwareMitigationBufferIndex = 0; // when FW is fixed this goes back to index 1
						mask >>= 8;
					}

					for (int i = 0; i < 8; ++i) {
						if (mask & 1) {
							mask = 0;
						}
						else {
							if (mask > 0) {
								beamDestination->bitShift += 4;
								mask >>= 1;
							}
						}
					}
				} catch(YAML::InvalidNode &e) {
					errorStream << "ERROR: Failed to find field " << field << " for BeamDestination.";
					throw(DbException(errorStream.str()));
				} catch(YAML::TypedBadConversion<unsigned int> &e) {
					errorStream << "ERROR: Failed to convert contents of field " << field << " for BeamDestination (expected unsigned int).";
					throw(DbException(errorStream.str()));
				} catch(YAML::TypedBadConversion<short> &e) {
					errorStream << "ERROR: Failed to convert contents of field " << field << " for BeamDestination (expected unsigned int).";
					throw(DbException(errorStream.str()));
				} catch(YAML::TypedBadConversion<std::string> &e) {
					errorStream << "ERROR: Failed to convert contents of field " << field << " for BeamDestination (expected string).";
					throw(DbException(errorStream.str()));
				}

				rhs->insert(std::pair<int, DbBeamDestinationPtr>(beamDestination->id,
											DbBeamDestinationPtr(beamDestination)));
					}

			return true;
		}
  	};


	/**
	 * BeamClass:
	 *   id: 1
	 * 	 number: 3
	 *   name: 'Kicker STBY'
	 * 	 integration_window: 455000
	 * 	 min_period: 0
	 *   total_charge: 0
	 */
  template<>
    struct convert<DbBeamClassMapPtr> {
		static bool decode(const Node &node, DbBeamClassMapPtr &rhs) {
			DbBeamClassMap *beamClasses = new DbBeamClassMap();
			std::stringstream errorStream;
			std::string field;
			rhs = DbBeamClassMapPtr(beamClasses);

			for (YAML::Node::const_iterator it = node["BeamClass"].begin();
				it != node["BeamClass"].end(); ++it) {
				DbBeamClass *beamClass = new DbBeamClass();

				try {
					field = "id";
					beamClass->id = (*it)[field].as<unsigned int>();

					field = "name";
					beamClass->name = (*it)[field].as<std::string>();

					field = "number";
					beamClass->number = (*it)[field].as<unsigned int>();

					field = "min_period";
					beamClass->minPeriod = (*it)[field].as<unsigned int>();

					field = "integration_window";
					beamClass->integrationWindow = (*it)[field].as<unsigned int>();

					field = "total_charge";
					beamClass->totalCharge = (*it)[field].as<unsigned int>();

				} catch(YAML::InvalidNode &e) {
					errorStream << "ERROR: Failed to find field " << field << " for BeamClass.";
					throw(DbException(errorStream.str()));
				} catch(YAML::TypedBadConversion<unsigned int> &e) {
					errorStream << "ERROR: Failed to convert contents of field " << field << " for BeamClass (expected unsigned int).";
					throw(DbException(errorStream.str()));
				} catch(YAML::TypedBadConversion<std::string> &e) {
					errorStream << "ERROR: Failed to convert contents of field " << field << " for BeamClass (expected string).";
					throw(DbException(errorStream.str()));
				}

				rhs->insert(std::pair<int, DbBeamClassPtr>(beamClass->id, DbBeamClassPtr(beamClass)));
			}

			return true;
		}
  	};

  /**
   * AllowedClass:
   * - beam_class_id: '1'
   *   fault_state_id: '1'
   *   id: '1'
   *   beam_destination_id: '1'
   */
  template<>
    struct convert<DbAllowedClassMapPtr> {
    static bool decode(const Node &node, DbAllowedClassMapPtr &rhs) {
      DbAllowedClassMap *allowedClasses = new DbAllowedClassMap();
      std::stringstream errorStream;
      std::string field;
      rhs = DbAllowedClassMapPtr(allowedClasses);

      for (YAML::Node::const_iterator it = node["AllowedClass"].begin();
	   it != node["AllowedClass"].end(); ++it) {
	DbAllowedClass *allowedClass = new DbAllowedClass();

	try {
	  field = "id";
	  allowedClass->id = (*it)[field].as<unsigned int>();

	  field = "beam_class_id";
	  allowedClass->beamClassId = (*it)[field].as<unsigned int>();

	  field = "fault_state_id";
	  allowedClass->faultStateId = (*it)[field].as<unsigned int>();

	  field = "beam_destination_id";
	  allowedClass->beamDestinationId = (*it)[field].as<unsigned int>();
  	} catch(YAML::InvalidNode &e) {
	  errorStream << "ERROR: Failed to find field " << field << " for AllowedClass.";
	  throw(DbException(errorStream.str()));
	} catch(YAML::TypedBadConversion<unsigned int> &e) {
	  errorStream << "ERROR: Failed to convert contents of field " << field << " for AllowedClass (expected unsigned int).";
	  throw(DbException(errorStream.str()));
	}

	rhs->insert(std::pair<int, DbAllowedClassPtr>(allowedClass->id,
						      DbAllowedClassPtr(allowedClass)));
      }

      return true;
    }
  };

}

#endif
