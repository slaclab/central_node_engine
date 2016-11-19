/**
 * central_node_yaml.h
 *
 * Template specializations to convert MPS YAML database into
 * C++ objects. The C++ classes are described in the file
 * central_node_database.h.
 */

#ifndef CENTRAL_NODE_YAML_H
#define CENTRAL_NODE_YAML_H

#include <central_node_database.h>
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
      rhs = DbCrateMapPtr(crates);

      for (YAML::Node::const_iterator it = node["Crate"].begin();
	   it != node["Crate"].end(); ++it) {
	DbCrate *crate = new DbCrate();
	
	crate->id = (*it)["id"].as<int>();
	crate->number = (*it)["number"].as<int>();
	crate->numSlots = (*it)["num_slots"].as<int>();
	crate->shelfNumber = (*it)["shelf_number"].as<int>();

	rhs->insert(std::pair<int, DbCratePtr>(crate->id, DbCratePtr(crate)));
      }

      return true;
    }
  };

  /**
   * ApplicationType:
   * - analog_channel_count: '3'
   *   analog_channel_size: '8'
   *   digital_channel_count: '4'
   *   digital_channel_size: '1'
   *   id: '1'
   *   name: Mixed Mode Link Node
   *   number: '0'
   */
  template<>
    struct convert<DbApplicationTypeMapPtr> {
    static bool decode(const Node &node, DbApplicationTypeMapPtr &rhs) {
      DbApplicationTypeMap *appTypes = new DbApplicationTypeMap();
      rhs = DbApplicationTypeMapPtr(appTypes);

      for (YAML::Node::const_iterator it = node["ApplicationType"].begin();
	   it != node["ApplicationType"].end(); ++it) {
	DbApplicationType *appType = new DbApplicationType();
	
	appType->id = (*it)["id"].as<int>();
	appType->number = (*it)["number"].as<int>();
	appType->analogChannelCount = (*it)["analog_channel_count"].as<int>();
	appType->analogChannelSize = (*it)["analog_channel_size"].as<int>();
	appType->digitalChannelCount = (*it)["digital_channel_count"].as<int>();
	appType->digitalChannelSize = (*it)["digital_channel_size"].as<int>();
	appType->description = (*it)["name"].as<std::string>();

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
   */
  template<>
    struct convert<DbApplicationCardMapPtr> {
    static bool decode(const Node &node, DbApplicationCardMapPtr &rhs) {
      DbApplicationCardMap *appCards = new DbApplicationCardMap();
      rhs = DbApplicationCardMapPtr(appCards);

      for (YAML::Node::const_iterator it = node["ApplicationCard"].begin();
	   it != node["ApplicationCard"].end(); ++it) {
	DbApplicationCard *appCard = new DbApplicationCard();
	
	appCard->id = (*it)["id"].as<int>();
	appCard->number = (*it)["number"].as<int>();
	appCard->crateId = (*it)["crate_id"].as<int>();
	appCard->slotNumber = (*it)["slot_number"].as<int>();
	appCard->applicationTypeId = (*it)["type_id"].as<int>();

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
	int s = node[key].size();
      } catch (InvalidNode &e) {
	key = "AnalogChannel";
      }

      DbChannelMap *channels = new DbChannelMap();
      rhs = DbChannelMapPtr(channels);

      for (YAML::Node::const_iterator it = node[key].begin();
	   it != node[key].end(); ++it) {
	DbChannel *channel = new DbChannel();
	
	channel->id = (*it)["id"].as<int>();
	channel->name = (*it)["name"].as<std::string>();
	channel->number = (*it)["number"].as<int>();
	channel->cardId = (*it)["card_id"].as<int>();

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
      rhs = DbDeviceTypeMapPtr(deviceTypes);

      for (YAML::Node::const_iterator it = node["DeviceType"].begin();
	   it != node["DeviceType"].end(); ++it) {
	DbDeviceType *deviceType = new DbDeviceType();
	
	deviceType->id = (*it)["id"].as<int>();
	deviceType->name = (*it)["name"].as<std::string>();

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
      rhs = DbDeviceStateMapPtr(deviceStates);

      for (YAML::Node::const_iterator it = node["DeviceState"].begin();
	   it != node["DeviceState"].end(); ++it) {
	DbDeviceState *	deviceState = new DbDeviceState();
	
	deviceState->id = (*it)["id"].as<int>();
	deviceState->value = (*it)["value"].as<int>();
	deviceState->mask = (*it)["mask"].as<uint32_t>();
	deviceState->deviceTypeId = (*it)["device_type_id"].as<int>();
	deviceState->name = (*it)["name"].as<std::string>();

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
      rhs = DbDigitalDeviceMapPtr(digitalDevices);

      for (YAML::Node::const_iterator it = node["DigitalDevice"].begin();
	   it != node["DigitalDevice"].end(); ++it) {
	DbDigitalDevice *digitalDevice = new DbDigitalDevice();
	
	digitalDevice->id = (*it)["id"].as<int>();
	digitalDevice->deviceTypeId = (*it)["device_type_id"].as<int>();
	digitalDevice->name = (*it)["name"].as<std::string>();
	digitalDevice->description = (*it)["description"].as<std::string>();
	digitalDevice->zPosition = (*it)["z_position"].as<float>();
	digitalDevice->value = 0;

	rhs->insert(std::pair<int, DbDigitalDevicePtr>(digitalDevice->id,
						       DbDigitalDevicePtr(digitalDevice)));
      }

      return true;
    }
  };

  /**
   * DeviceInput:
   * - bit_position: '0'
   *   channel_id: '1'
   *   digital_device_id: '1'
   *   id: '1'
   */
  template<>
    struct convert<DbDeviceInputMapPtr> {
    static bool decode(const Node &node, DbDeviceInputMapPtr &rhs) {
      DbDeviceInputMap *deviceInputs = new DbDeviceInputMap();
      rhs = DbDeviceInputMapPtr(deviceInputs);

      for (YAML::Node::const_iterator it = node["DeviceInput"].begin();
	   it != node["DeviceInput"].end(); ++it) {
	DbDeviceInput *deviceInput = new DbDeviceInput();
	
	deviceInput->id = (*it)["id"].as<int>();
	deviceInput->bitPosition = (*it)["bit_position"].as<int>();
	deviceInput->digitalDeviceId = (*it)["digital_device_id"].as<int>();
	deviceInput->channelId = (*it)["channel_id"].as<int>();
	deviceInput->value = 0;

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
      rhs = DbConditionMapPtr(conditions);

      for (YAML::Node::const_iterator it = node["Condition"].begin();
	   it != node["Condition"].end(); ++it) {
	DbCondition *condition = new DbCondition();
	
	condition->id = (*it)["id"].as<int>();
	condition->name = (*it)["name"].as<std::string>();
	condition->description = (*it)["description"].as<std::string>();
	condition->mask = (*it)["value"].as<int>();

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
      rhs = DbIgnoreConditionMapPtr(ignoreConditions);

      for (YAML::Node::const_iterator it = node["IgnoreCondition"].begin();
	   it != node["IgnoreCondition"].end(); ++it) {
	DbIgnoreCondition *ignoreCondition = new DbIgnoreCondition();
	
	ignoreCondition->id = (*it)["id"].as<int>();
	ignoreCondition->faultStateId = (*it)["fault_state_id"].as<int>();
	ignoreCondition->conditionId = (*it)["condition_id"].as<int>();

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
      rhs = DbConditionInputMapPtr(conditionInputs);

      for (YAML::Node::const_iterator it = node["ConditionInput"].begin();
	   it != node["ConditionInput"].end(); ++it) {
	DbConditionInput *conditionInput = new DbConditionInput();
	
	conditionInput->id = (*it)["id"].as<int>();
	conditionInput->bitPosition = (*it)["bit_position"].as<int>();
	conditionInput->faultStateId = (*it)["fault_state_id"].as<int>();
	conditionInput->conditionId = (*it)["condition_id"].as<int>();

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
      rhs = DbFaultMapPtr(faults);

      for (YAML::Node::const_iterator it = node["Fault"].begin();
	   it != node["Fault"].end(); ++it) {
	DbFault *fault = new DbFault();
	
	fault->id = (*it)["id"].as<int>();
	fault->name = (*it)["name"].as<std::string>();
	fault->description = (*it)["description"].as<std::string>();
	fault->value = 0;

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
      rhs = DbFaultInputMapPtr(faultInputs);

      for (YAML::Node::const_iterator it = node["FaultInput"].begin();
	   it != node["FaultInput"].end(); ++it) {
	DbFaultInput *faultInput = new DbFaultInput();
	
	faultInput->id = (*it)["id"].as<int>();
	faultInput->bitPosition = (*it)["bit_position"].as<int>();
	faultInput->deviceId = (*it)["device_id"].as<int>();
	faultInput->faultId = (*it)["fault_id"].as<int>();
	faultInput->value = 0;

	rhs->insert(std::pair<int, DbFaultInputPtr>(faultInput->id,
						    DbFaultInputPtr(faultInput)));
      }

      return true;
    }
  };

  /**
   * DigitalFaultState:
   * - fault_id: '1'
   *   id: '1'
   *   name: Out
   *   value: '1'
   */
  template<>
    struct convert<DbDigitalFaultStateMapPtr> {
    static bool decode(const Node &node, DbDigitalFaultStateMapPtr &rhs) {
      DbDigitalFaultStateMap *digitalFaultStates = new DbDigitalFaultStateMap();
      rhs = DbDigitalFaultStateMapPtr(digitalFaultStates);

      for (YAML::Node::const_iterator it = node["DigitalFaultState"].begin();
	   it != node["DigitalFaultState"].end(); ++it) {
	DbDigitalFaultState *digitalFaultState = new DbDigitalFaultState();
	
	digitalFaultState->id = (*it)["id"].as<int>();
	//	digitalFaultState->value = (*it)["value"].as<int>();
	digitalFaultState->faultId = (*it)["fault_id"].as<int>();
	//	digitalFaultState->name = (*it)["name"].as<std::string>();
	digitalFaultState->deviceStateId = (*it)["device_state_id"].as<int>();
	digitalFaultState->defaultState = (*it)["default"].as<bool>();

	rhs->insert(std::pair<int, DbDigitalFaultStatePtr>(digitalFaultState->id,
							   DbDigitalFaultStatePtr(digitalFaultState)));
      }

      return true;
    }
  };

  /**
   * ### UNUSED ###
   *
   * ThresholdValueMap:
   * - description: Map for generic PICs.
   *   id: '1'
   */
  template<>
    struct convert<DbThresholdValueMapEntryMapPtr> {
    static bool decode(const Node &node, DbThresholdValueMapEntryMapPtr &rhs) {
      DbThresholdValueMapEntryMap *thresValueMaps = new DbThresholdValueMapEntryMap();
      rhs = DbThresholdValueMapEntryMapPtr(thresValueMaps);

      for (YAML::Node::const_iterator it = node["ThresholdValueMap"].begin();
	   it != node["ThresholdValueMap"].end(); ++it) {
	DbThresholdValueMapEntry *thresValueMap = new DbThresholdValueMapEntry();
	
	thresValueMap->id = (*it)["id"].as<int>();
	thresValueMap->description = (*it)["description"].as<std::string>();

	rhs->insert(std::pair<int, DbThresholdValueMapEntryPtr>(thresValueMap->id,
								DbThresholdValueMapEntryPtr(thresValueMap)));
      }

      return true;
    }
  };

  /**
   * ### UNUSED ###
   *
   * ThresholdValue:
   * - id: '1'
   *   threshold: '0'
   *   threshold_value_map_id: '1'
   *   value: '0.0'
   */
  template<>
    struct convert<DbThresholdValueMapPtr> {
    static bool decode(const Node &node, DbThresholdValueMapPtr &rhs) {
      DbThresholdValueMap *thresValues = new DbThresholdValueMap();
      rhs = DbThresholdValueMapPtr(thresValues);

      for (YAML::Node::const_iterator it = node["ThresholdValue"].begin();
	   it != node["ThresholdValue"].end(); ++it) {
	DbThresholdValue *thresValue = new DbThresholdValue();
	
	thresValue->id = (*it)["id"].as<int>();
	thresValue->threshold = (*it)["threshold"].as<int>();
	thresValue->thresholdValueMapId = (*it)["threshold_value_map_id"].as<int>();
	thresValue->value = (*it)["value"].as<float>();

	rhs->insert(std::pair<int, DbThresholdValuePtr>(thresValue->id, 
							DbThresholdValuePtr(thresValue)));
      }

      return true;
    }
  };

  /**
   * ### UNUSED ###
   *
   * ThresholdFault:
   * - analog_device_id: '3'
   *   greater_than: 'True'
   *   id: '1'
   *   name: PIC Loss > 1.0
   *   threshold: '1.0'
   */
  template<>
    struct convert<DbThresholdFaultMapPtr> {
    static bool decode(const Node &node, DbThresholdFaultMapPtr &rhs) {
      DbThresholdFaultMap *thresFaults = new DbThresholdFaultMap();
      rhs = DbThresholdFaultMapPtr(thresFaults);

      for (YAML::Node::const_iterator it = node["ThresholdFault"].begin();
	   it != node["ThresholdFault"].end(); ++it) {
	DbThresholdFault *thresFault = new DbThresholdFault();
	
	thresFault->id = (*it)["id"].as<int>();
	thresFault->analogDeviceId = (*it)["analog_device_id"].as<int>();
	thresFault->name = (*it)["name"].as<std::string>();
	thresFault->thresholdValueId = (*it)["threshold_value_id"].as<int>();
	//	thresFault->threshold = (*it)["threshold"].as<int>();
	
	std::string greaterThanString = (*it)["greater_than"].as<std::string>();
	if (greaterThanString == "True") {
	  thresFault->greaterThan = true;
	} else {
	  thresFault->greaterThan = false;
	}

	rhs->insert(std::pair<int, DbThresholdFaultPtr>(thresFault->id,
							DbThresholdFaultPtr(thresFault)));
      }

      return true;
    }
  };


  /**
   * ### UNUSED ###
   *
   * AnalogDeviceType:
   * - id: '1'
   *   name: PIC
   *   threshold_value_map_id: '1'
   *   units: counts
   */
  template<>
    struct convert<DbAnalogDeviceTypeMapPtr> {
    static bool decode(const Node &node, DbAnalogDeviceTypeMapPtr &rhs) {
      DbAnalogDeviceTypeMap *analogDeviceTypes = new DbAnalogDeviceTypeMap();
      rhs = DbAnalogDeviceTypeMapPtr(analogDeviceTypes);

      for (YAML::Node::const_iterator it = node["AnalogDeviceType"].begin();
	   it != node["AnalogDeviceType"].end(); ++it) {
	DbAnalogDeviceType *analogDeviceType = new DbAnalogDeviceType();
	
	analogDeviceType->id = (*it)["id"].as<int>();
	analogDeviceType->name = (*it)["name"].as<std::string>();
	analogDeviceType->units = (*it)["units"].as<std::string>();
	analogDeviceType->thresholdValueMapId = (*it)["threshold_value_map_id"].as<int>();

	rhs->insert(std::pair<int, DbAnalogDeviceTypePtr>(analogDeviceType->id,
							  DbAnalogDeviceTypePtr(analogDeviceType)));
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
      rhs = DbAnalogDeviceMapPtr(analogDevices);

      for (YAML::Node::const_iterator it = node["AnalogDevice"].begin();
	   it != node["AnalogDevice"].end(); ++it) {
	DbAnalogDevice *analogDevice = new DbAnalogDevice();
	
	analogDevice->id = (*it)["id"].as<int>();
	analogDevice->deviceTypeId = (*it)["device_type_id"].as<int>();
	analogDevice->channelId = (*it)["channel_id"].as<int>();
	analogDevice->name = (*it)["name"].as<std::string>();
	analogDevice->description = (*it)["description"].as<std::string>();
	analogDevice->zPosition = (*it)["z_position"].as<float>();
	analogDevice->value = 0;

	rhs->insert(std::pair<int, DbAnalogDevicePtr>(analogDevice->id,
						      DbAnalogDevicePtr(analogDevice)));
      }

      return true;
    }
  };

  /**
   * ### UNUSED ###
   *
   * ThresholdFaultState:
   * - id: '8'
   *   threshold_fault_id: '1'
   */
  template<>
    struct convert<DbThresholdFaultStateMapPtr> {
    static bool decode(const Node &node, DbThresholdFaultStateMapPtr &rhs) {
      DbThresholdFaultStateMap *thresFaultStates = new DbThresholdFaultStateMap();
      rhs = DbThresholdFaultStateMapPtr(thresFaultStates);

      for (YAML::Node::const_iterator it = node["ThresholdFaultState"].begin();
	   it != node["ThresholdFaultState"].end(); ++it) {
	DbThresholdFaultState *thresFaultState = new DbThresholdFaultState();
	
	thresFaultState->id = (*it)["id"].as<int>();
	thresFaultState->thresholdFaultId = (*it)["threshold_fault_id"].as<int>();

	rhs->insert(std::pair<int, DbThresholdFaultStatePtr>(thresFaultState->id,
							     DbThresholdFaultStatePtr(thresFaultState)));
      }

      return true;
    }
  };

  /**
   * MitigationDevice:
   * - id: '1'
   *   name: Gun
   */
  template<>
    struct convert<DbMitigationDeviceMapPtr> {
    static bool decode(const Node &node, DbMitigationDeviceMapPtr &rhs) {
      DbMitigationDeviceMap *mitigationDevices = new DbMitigationDeviceMap();
      rhs = DbMitigationDeviceMapPtr(mitigationDevices);

      for (YAML::Node::const_iterator it = node["MitigationDevice"].begin();
	   it != node["MitigationDevice"].end(); ++it) {
	DbMitigationDevice *mitigationDevice = new DbMitigationDevice();
	
	mitigationDevice->id = (*it)["id"].as<int>();
	mitigationDevice->name = (*it)["name"].as<std::string>();

	rhs->insert(std::pair<int, DbMitigationDevicePtr>(mitigationDevice->id, 
							  DbMitigationDevicePtr(mitigationDevice)));
      }

      return true;
    }
  };

  /**
   * BeamClass:
   * - id: '1'
   *   name: Class 1
   *   number: '1'
   */
  template<>
    struct convert<DbBeamClassMapPtr> {
    static bool decode(const Node &node, DbBeamClassMapPtr &rhs) {
      DbBeamClassMap *beamClasses = new DbBeamClassMap();
      rhs = DbBeamClassMapPtr(beamClasses);

      for (YAML::Node::const_iterator it = node["BeamClass"].begin();
	   it != node["BeamClass"].end(); ++it) {
	DbBeamClass *beamClass = new DbBeamClass();
	
	beamClass->id = (*it)["id"].as<int>();
	beamClass->name = (*it)["name"].as<std::string>();
	beamClass->number = (*it)["number"].as<int>();

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
   *   mitigation_device_id: '1'
   */
  template<>
    struct convert<DbAllowedClassMapPtr> {
    static bool decode(const Node &node, DbAllowedClassMapPtr &rhs) {
      DbAllowedClassMap *allowedClasses = new DbAllowedClassMap();
      rhs = DbAllowedClassMapPtr(allowedClasses);

      for (YAML::Node::const_iterator it = node["AllowedClass"].begin();
	   it != node["AllowedClass"].end(); ++it) {
	DbAllowedClass *allowedClass = new DbAllowedClass();
	
	allowedClass->id = (*it)["id"].as<int>();
	allowedClass->beamClassId = (*it)["beam_class_id"].as<int>();
	allowedClass->faultStateId = (*it)["fault_state_id"].as<int>();
	allowedClass->mitigationDeviceId = (*it)["mitigation_device_id"].as<int>();

	rhs->insert(std::pair<int, DbAllowedClassPtr>(allowedClass->id,
						      DbAllowedClassPtr(allowedClass)));
      }

      return true;
    }
  };

}

#endif
