/**
 * central_node_yaml.h
 *
 * Template specializations to convert MPS YAML database into
 * c++ objects.
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
    struct convert<DbCrateListPtr> {
    static bool decode(const Node &node, DbCrateListPtr &rhs) {
      size_t elements = node["Crate"].size();

      DbCrateList *crates = new DbCrateList(elements + 1);
      rhs = DbCrateListPtr(crates);

      DbCrate *crate = new DbCrate();
      rhs->at(0) = DbCratePtr(crate);

      for (YAML::Node::const_iterator it = node["Crate"].begin();
	   it != node["Crate"].end(); ++it) {
	crate = new DbCrate();
	
	crate->id = (*it)["id"].as<int>();
	crate->number = (*it)["number"].as<int>();
	crate->numSlots = (*it)["num_slots"].as<int>();
	crate->shelfNumber = (*it)["shelf_number"].as<int>();

	rhs->at(crate->id) = DbCratePtr(crate);
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
    struct convert<DbApplicationTypeListPtr> {
    static bool decode(const Node &node, DbApplicationTypeListPtr &rhs) {
      size_t elements = node["ApplicationType"].size();

      DbApplicationTypeList *appTypes = new DbApplicationTypeList(elements + 1);
      rhs = DbApplicationTypeListPtr(appTypes);

      DbApplicationType *appType = new DbApplicationType();
      rhs->at(0) = DbApplicationTypePtr(appType);

      for (YAML::Node::const_iterator it = node["ApplicationType"].begin();
	   it != node["ApplicationType"].end(); ++it) {
	appType = new DbApplicationType();
	
	appType->id = (*it)["id"].as<int>();
	appType->number = (*it)["number"].as<int>();
	appType->analogChannelCount = (*it)["analog_channel_count"].as<int>();
	appType->analogChannelSize = (*it)["analog_channel_size"].as<int>();
	appType->digitalChannelCount = (*it)["digital_channel_count"].as<int>();
	appType->digitalChannelSize = (*it)["digital_channel_size"].as<int>();
	appType->description = (*it)["name"].as<std::string>();

	rhs->at(appType->id) = DbApplicationTypePtr(appType);
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
    struct convert<DbApplicationCardListPtr> {
    static bool decode(const Node &node, DbApplicationCardListPtr &rhs) {
      size_t elements = node["ApplicationCard"].size();

      DbApplicationCardList *appCards = new DbApplicationCardList(elements + 1);
      rhs = DbApplicationCardListPtr(appCards);

      DbApplicationCard *appCard = new DbApplicationCard();
      rhs->at(0) = DbApplicationCardPtr(appCard);

      for (YAML::Node::const_iterator it = node["ApplicationCard"].begin();
	   it != node["ApplicationCard"].end(); ++it) {
	DbApplicationCard *appCard = new DbApplicationCard();
	
	appCard->id = (*it)["id"].as<int>();
	appCard->number = (*it)["number"].as<int>();
	appCard->crateId = (*it)["crate_id"].as<int>();
	appCard->slotNumber = (*it)["slot_number"].as<int>();
	appCard->applicationTypeId = (*it)["type_id"].as<int>();

	rhs->at(appCard->id) = DbApplicationCardPtr(appCard);
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
    struct convert<DbChannelListPtr> {
    static bool decode(const Node &node, DbChannelListPtr &rhs) {
      size_t elements = -1;
      std::string key = "";

      try {
	key = "DigitalChannel";
	elements = node[key].size();
      } catch (InvalidNode &e) {
	key = "";
      }
      
      if (key == "") {
	try {
	  key = "AnalogChannel";
	  elements = node[key].size();
	} catch (InvalidNode &e) {
	  throw e;
	}
      }

      DbChannelList *channels = new DbChannelList(elements + 1);
      rhs = DbChannelListPtr(channels);

      DbChannel *channel = new DbChannel();
      rhs->at(0) = DbChannelPtr(channel);

      for (YAML::Node::const_iterator it = node[key].begin();
	   it != node[key].end(); ++it) {
	DbChannel *channel = new DbChannel();
	
	channel->id = (*it)["id"].as<int>();
	channel->number = (*it)["number"].as<int>();
	channel->cardId = (*it)["card_id"].as<int>();

	rhs->at(channel->id) = DbChannelPtr(channel);
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
    struct convert<DbDeviceTypeListPtr> {
    static bool decode(const Node &node, DbDeviceTypeListPtr &rhs) {
      size_t elements = node["DeviceType"].size();

      DbDeviceTypeList *deviceTypes = new DbDeviceTypeList(elements + 1);
      rhs = DbDeviceTypeListPtr(deviceTypes);

      DbDeviceType *deviceType = new DbDeviceType();
      rhs->at(0) = DbDeviceTypePtr(deviceType);

      for (YAML::Node::const_iterator it = node["DeviceType"].begin();
	   it != node["DeviceType"].end(); ++it) {
	deviceType = new DbDeviceType();
	
	deviceType->id = (*it)["id"].as<int>();
	deviceType->name = (*it)["name"].as<std::string>();

	rhs->at(deviceType->id) = DbDeviceTypePtr(deviceType);
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
    struct convert<DbDeviceStateListPtr> {
    static bool decode(const Node &node, DbDeviceStateListPtr &rhs) {
      size_t elements = node["DeviceState"].size();

      DbDeviceStateList *deviceStates = new DbDeviceStateList(elements + 1);
      rhs = DbDeviceStateListPtr(deviceStates);

      DbDeviceState *deviceState = new DbDeviceState();
      rhs->at(0) = DbDeviceStatePtr(deviceState);

      for (YAML::Node::const_iterator it = node["DeviceState"].begin();
	   it != node["DeviceState"].end(); ++it) {
	deviceState = new DbDeviceState();
	
	deviceState->id = (*it)["id"].as<int>();
	deviceState->value = (*it)["value"].as<int>();
	deviceState->deviceTypeId = (*it)["device_type_id"].as<int>();
	deviceState->name = (*it)["name"].as<std::string>();

	rhs->at(deviceState->id) = DbDeviceStatePtr(deviceState);
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
    struct convert<DbDigitalDeviceListPtr> {
    static bool decode(const Node &node, DbDigitalDeviceListPtr &rhs) {
      size_t elements = node["DigitalDevice"].size();

      DbDigitalDeviceList *digitalDevices = new DbDigitalDeviceList(elements + 1);
      rhs = DbDigitalDeviceListPtr(digitalDevices);

      DbDigitalDevice *digitalDevice = new DbDigitalDevice();
      rhs->at(0) = DbDigitalDevicePtr(digitalDevice);

      for (YAML::Node::const_iterator it = node["DigitalDevice"].begin();
	   it != node["DigitalDevice"].end(); ++it) {
	digitalDevice = new DbDigitalDevice();
	
	digitalDevice->id = (*it)["id"].as<int>();
	digitalDevice->deviceTypeId = (*it)["device_type_id"].as<int>();

	rhs->at(digitalDevice->id) = DbDigitalDevicePtr(digitalDevice);
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
    struct convert<DbDeviceInputListPtr> {
    static bool decode(const Node &node, DbDeviceInputListPtr &rhs) {
      size_t elements = node["DeviceInput"].size();

      DbDeviceInputList *deviceInputs = new DbDeviceInputList(elements + 1);
      rhs = DbDeviceInputListPtr(deviceInputs);

      DbDeviceInput *deviceInput = new DbDeviceInput();
      rhs->at(0) = DbDeviceInputPtr(deviceInput);

      for (YAML::Node::const_iterator it = node["DeviceInput"].begin();
	   it != node["DeviceInput"].end(); ++it) {
	deviceInput = new DbDeviceInput();
	
	deviceInput->id = (*it)["id"].as<int>();
	deviceInput->bitPosition = (*it)["bit_position"].as<int>();
	deviceInput->digitalDeviceId = (*it)["digital_device_id"].as<int>();
	deviceInput->channelId = (*it)["channel_id"].as<int>();

	rhs->at(deviceInput->id) = DbDeviceInputPtr(deviceInput);
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
    struct convert<DbFaultListPtr> {
    static bool decode(const Node &node, DbFaultListPtr &rhs) {
      size_t elements = node["Fault"].size();

      DbFaultList *faults = new DbFaultList(elements + 1);
      rhs = DbFaultListPtr(faults);

      DbFault *fault = new DbFault();
      rhs->at(0) = DbFaultPtr(fault);

      for (YAML::Node::const_iterator it = node["Fault"].begin();
	   it != node["Fault"].end(); ++it) {
	fault = new DbFault();
	
	fault->id = (*it)["id"].as<int>();
	fault->name = (*it)["name"].as<std::string>();
	fault->description = (*it)["description"].as<std::string>();

	rhs->at(fault->id) = DbFaultPtr(fault);
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
    struct convert<DbFaultInputListPtr> {
    static bool decode(const Node &node, DbFaultInputListPtr &rhs) {
      size_t elements = node["FaultInput"].size();

      DbFaultInputList *faultInputs = new DbFaultInputList(elements + 1);
      rhs = DbFaultInputListPtr(faultInputs);

      DbFaultInput *faultInput = new DbFaultInput();
      rhs->at(0) = DbFaultInputPtr(faultInput);

      for (YAML::Node::const_iterator it = node["FaultInput"].begin();
	   it != node["FaultInput"].end(); ++it) {
	faultInput = new DbFaultInput();
	
	faultInput->id = (*it)["id"].as<int>();
	faultInput->bitPosition = (*it)["bit_position"].as<int>();
	faultInput->deviceId = (*it)["device_id"].as<int>();
	faultInput->faultId = (*it)["fault_id"].as<int>();

	rhs->at(faultInput->id) = DbFaultInputPtr(faultInput);
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
    struct convert<DbDigitalFaultStateListPtr> {
    static bool decode(const Node &node, DbDigitalFaultStateListPtr &rhs) {
      size_t elements = node["DigitalFaultState"].size();

      DbDigitalFaultStateList *digitalFaultStates = new DbDigitalFaultStateList(elements + 1);
      rhs = DbDigitalFaultStateListPtr(digitalFaultStates);

      DbDigitalFaultState *digitalFaultState = new DbDigitalFaultState();
      rhs->at(0) = DbDigitalFaultStatePtr(digitalFaultState);

      for (YAML::Node::const_iterator it = node["DigitalFaultState"].begin();
	   it != node["DigitalFaultState"].end(); ++it) {
	digitalFaultState = new DbDigitalFaultState();
	
	digitalFaultState->id = (*it)["id"].as<int>();
	digitalFaultState->value = (*it)["value"].as<int>();
	digitalFaultState->faultId = (*it)["fault_id"].as<int>();
	digitalFaultState->name = (*it)["name"].as<std::string>();

	rhs->at(digitalFaultState->id) = DbDigitalFaultStatePtr(digitalFaultState);
      }

      return true;
    }
  };

  /**
   * ThresholdValueMap:
   * - description: Map for generic PICs.
   *   id: '1'
   */
  template<>
    struct convert<DbThresholdValueMapListPtr> {
    static bool decode(const Node &node, DbThresholdValueMapListPtr &rhs) {
      size_t elements = node["ThresholdValueMap"].size();

      DbThresholdValueMapList *thresValueMaps = new DbThresholdValueMapList(elements + 1);
      rhs = DbThresholdValueMapListPtr(thresValueMaps);

      DbThresholdValueMap *thresValueMap = new DbThresholdValueMap();
      rhs->at(0) = DbThresholdValueMapPtr(thresValueMap);

      for (YAML::Node::const_iterator it = node["ThresholdValueMap"].begin();
	   it != node["ThresholdValueMap"].end(); ++it) {
	thresValueMap = new DbThresholdValueMap();
	
	thresValueMap->id = (*it)["id"].as<int>();
	thresValueMap->description = (*it)["description"].as<std::string>();

	rhs->at(thresValueMap->id) = DbThresholdValueMapPtr(thresValueMap);
      }

      return true;
    }
  };

  /**
   * ThresholdValue:
   * - id: '1'
   *   threshold: '0'
   *   threshold_value_map_id: '1'
   *   value: '0.0'
   */
  template<>
    struct convert<DbThresholdValueListPtr> {
    static bool decode(const Node &node, DbThresholdValueListPtr &rhs) {
      size_t elements = node["ThresholdValue"].size();

      DbThresholdValueList *thresValues = new DbThresholdValueList(elements + 1);
      rhs = DbThresholdValueListPtr(thresValues);

      DbThresholdValue *thresValue = new DbThresholdValue();
      rhs->at(0) = DbThresholdValuePtr(thresValue);

      for (YAML::Node::const_iterator it = node["ThresholdValue"].begin();
	   it != node["ThresholdValue"].end(); ++it) {
	thresValue = new DbThresholdValue();
	
	thresValue->id = (*it)["id"].as<int>();
	thresValue->threshold = (*it)["threshold"].as<int>();
	thresValue->thresholdValueMapId = (*it)["threshold_value_map_id"].as<int>();
	thresValue->value = (*it)["value"].as<float>();

	rhs->at(thresValue->id) = DbThresholdValuePtr(thresValue);
      }

      return true;
    }
  };

  /**
   * ThresholdFault:
   * - analog_device_id: '3'
   *   greater_than: 'True'
   *   id: '1'
   *   name: PIC Loss > 1.0
   *   threshold: '1.0'
   */
  template<>
    struct convert<DbThresholdFaultListPtr> {
    static bool decode(const Node &node, DbThresholdFaultListPtr &rhs) {
      size_t elements = node["ThresholdFault"].size();

      DbThresholdFaultList *thresFaults = new DbThresholdFaultList(elements + 1);
      rhs = DbThresholdFaultListPtr(thresFaults);

      DbThresholdFault *thresFault = new DbThresholdFault();
      rhs->at(0) = DbThresholdFaultPtr(thresFault);

      for (YAML::Node::const_iterator it = node["ThresholdFault"].begin();
	   it != node["ThresholdFault"].end(); ++it) {
	thresFault = new DbThresholdFault();
	
	thresFault->id = (*it)["id"].as<int>();
	thresFault->analogDeviceId = (*it)["analog_device_id"].as<int>();
	thresFault->name = (*it)["name"].as<std::string>();
	thresFault->threshold = (*it)["threshold"].as<float>();
	
	std::string greaterThanString = (*it)["greater_than"].as<std::string>();
	if (greaterThanString == "True") {
	  thresFault->greaterThan = true;
	} else {
	  thresFault->greaterThan = false;
	}

	rhs->at(thresFault->id) = DbThresholdFaultPtr(thresFault);
      }

      return true;
    }
  };

  /**
   * ThresholdFaultState:
   * - id: '8'
   *   threshold_fault_id: '1'
   */
  template<>
    struct convert<DbThresholdFaultStateListPtr> {
    static bool decode(const Node &node, DbThresholdFaultStateListPtr &rhs) {
      size_t elements = node["ThresholdFaultState"].size();

      DbThresholdFaultStateList *thresFaultStates = new DbThresholdFaultStateList(elements + 1);
      rhs = DbThresholdFaultStateListPtr(thresFaultStates);

      DbThresholdFaultState *thresFaultState = new DbThresholdFaultState();
      rhs->at(0) = DbThresholdFaultStatePtr(thresFaultState);

      for (YAML::Node::const_iterator it = node["ThresholdFaultState"].begin();
	   it != node["ThresholdFaultState"].end(); ++it) {
	thresFaultState = new DbThresholdFaultState();
	
	thresFaultState->id = (*it)["id"].as<int>();
	thresFaultState->thresholdFaultId = (*it)["threshold_fault_id"].as<int>();

	std::cout << "ID: " << thresFaultState->id << std::endl;

	rhs->at(thresFaultState->id) = DbThresholdFaultStatePtr(thresFaultState);
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
    struct convert<DbMitigationDeviceListPtr> {
    static bool decode(const Node &node, DbMitigationDeviceListPtr &rhs) {
      size_t elements = node["MitigationDevice"].size();

      DbMitigationDeviceList *mitigationDevices = new DbMitigationDeviceList(elements + 1);
      rhs = DbMitigationDeviceListPtr(mitigationDevices);

      DbMitigationDevice *mitigationDevice = new DbMitigationDevice();
      rhs->at(0) = DbMitigationDevicePtr(mitigationDevice);

      for (YAML::Node::const_iterator it = node["MitigationDevice"].begin();
	   it != node["MitigationDevice"].end(); ++it) {
	mitigationDevice = new DbMitigationDevice();
	
	mitigationDevice->id = (*it)["id"].as<int>();
	mitigationDevice->name = (*it)["name"].as<std::string>();

	rhs->at(mitigationDevice->id) = DbMitigationDevicePtr(mitigationDevice);
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
    struct convert<DbBeamClassListPtr> {
    static bool decode(const Node &node, DbBeamClassListPtr &rhs) {
      size_t elements = node["BeamClass"].size();

      DbBeamClassList *beamClasses = new DbBeamClassList(elements + 1);
      rhs = DbBeamClassListPtr(beamClasses);

      DbBeamClass *beamClass = new DbBeamClass();
      rhs->at(0) = DbBeamClassPtr(beamClass);

      for (YAML::Node::const_iterator it = node["BeamClass"].begin();
	   it != node["BeamClass"].end(); ++it) {
	beamClass = new DbBeamClass();
	
	beamClass->id = (*it)["id"].as<int>();
	beamClass->name = (*it)["name"].as<std::string>();
	beamClass->number = (*it)["number"].as<int>();

	rhs->at(beamClass->id) = DbBeamClassPtr(beamClass);
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
    struct convert<DbAllowedClassListPtr> {
    static bool decode(const Node &node, DbAllowedClassListPtr &rhs) {
      size_t elements = node["AllowedClass"].size();

      DbAllowedClassList *allowedClasses = new DbAllowedClassList(elements + 1);
      rhs = DbAllowedClassListPtr(allowedClasses);

      DbAllowedClass *allowedClass = new DbAllowedClass();
      rhs->at(0) = DbAllowedClassPtr(allowedClass);

      for (YAML::Node::const_iterator it = node["AllowedClass"].begin();
	   it != node["AllowedClass"].end(); ++it) {
	allowedClass = new DbAllowedClass();
	
	allowedClass->id = (*it)["id"].as<int>();
	allowedClass->beamClassId = (*it)["beam_class_id"].as<int>();
	allowedClass->faultStateId = (*it)["fault_state_id"].as<int>();
	allowedClass->mitigationDeviceId = (*it)["mitigation_device_id"].as<int>();

	rhs->at(allowedClass->id) = DbAllowedClassPtr(allowedClass);
      }

      return true;
    }
  };

}

#endif
