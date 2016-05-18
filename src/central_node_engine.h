#ifndef CENTRAL_NODE_ENGINE_H
#define CENTRAL_NODE_ENGINE_H

#include <iostream>

#include <boost/shared_ptr.hpp>
#include <central_node_yaml.h>
#include <central_node_database.h>

using boost::shared_ptr;

/**
 * Evaluation Cycle:
 * - Read new info from firmware (inputs and mitigation info)
 * - Update values for digital/analog inputs
 * - Update values for digital/analog faults
 * - Check allowed beam at mitigation devices
 * - Request mitigation
 */

class Engine {
 public:
  Engine();

  int loadConfig(std::string yamlFileName);
  int checkFaults();

  friend class EngineTest;

 protected:
  int updateInputs();

  shared_ptr<MpsDb> mpsDb;

  DbBeamClassPtr highestBeamClass;
  DbBeamClassPtr lowestBeamClass;
};

typedef shared_ptr<Engine> EnginePtr;

#endif
