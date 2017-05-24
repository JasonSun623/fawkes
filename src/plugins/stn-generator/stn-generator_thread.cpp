
/***************************************************************************
 *  stn-generator_thread.cpp - stn-generator
 *
 *  Created: Sat May  6 20:16:21 2017
 *  Copyright  2017  Matthias Loebach
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL file in the doc directory.
 */

#include <fstream>
#include <streambuf>
#include <utils/misc/string_conversions.h>

#include "stn-generator_thread.h"

using namespace fawkes;
using namespace mongo;

/** @class StnGeneratorThread 'stn-generator_thread.h' 
 * Generates an STN representation of a sequential task plan
 * @author Matthias Loebach
 */

/** Constructor. */
StnGeneratorThread::StnGeneratorThread()
 : Thread("StnGeneratorThread", Thread::OPMODE_WAITFORWAKEUP),
  BlackBoardInterfaceListener("StnGeneratorThread")
{
}

void
StnGeneratorThread::init()
{
  logger->log_info(name(),"reading config");
  std::string cfg_prefix = "plugins/stn-generator/";
  cfg_plan_collection_ = config->get_string(cfg_prefix + "plan/collection");
  cfg_output_collection_ = config->get_string(cfg_prefix + "output/collection");

  std::string pddl_domain_path = StringConversions::resolve_path(
      config->get_string(cfg_prefix + "domain-file"));
  logger->log_info(name(),"Reading domain from %s", pddl_domain_path.c_str());
  std::ifstream s(pddl_domain_path);
  std::string pddl_domain;

  s.seekg(0, std::ios::end);
  pddl_domain.reserve(s.tellg());
  s.seekg(0, std::ios::beg);
  pddl_domain.assign((std::istreambuf_iterator<char>(s)),
      std::istreambuf_iterator<char>());

  if ( config->get_bool(cfg_prefix + "generate-classic-domain") ) {
    std::string classic_dom_path = StringConversions::resolve_path(
      config->get_string(cfg_prefix + "classic-domain-file"));
    stn_ = new stn::Stn(logger, classic_dom_path);
  } else {
    stn_ = new stn::Stn(logger);
  }
  stn_->set_pddl_domain(pddl_domain);
  logger->log_info(name(),"Created STN object from domain");

  plan_if_ = blackboard->open_for_reading<PddlPlannerInterface>(config->get_string(cfg_prefix + "plan/interface").c_str());
  bbil_add_data_interface(plan_if_);
  blackboard->register_listener(this, BlackBoard::BBIL_FLAG_DATA);
}

void
StnGeneratorThread::loop()
{
  //TODO REMOVE AFTER READING DOMAIN
  stn_->set_initial_state(stn::StnAction(std::string("init"),
                    {},
                    {stn::Predicate("at", true, {"a","pool1"}),
                     stn::Predicate("at", true, {"b","pool1"}),
                     stn::Predicate("at", true, {"c","pool1"}),
                     stn::Predicate("at", true, {"d","pool1"}),
                     stn::Predicate("at", true, {"e","pool1"}),
                     stn::Predicate("at", true, {"y","pool2"}),
                     stn::Predicate("at", true, {"z","pool2"}),
                     stn::Predicate("clear", true, {"a"}),
                     stn::Predicate("clear", true, {"b"}),
                     stn::Predicate("clear", true, {"c"}),
                     stn::Predicate("clear", true, {"d"}),
                     stn::Predicate("clear", true, {"e"})
                    },
                    std::string("")
                  ));
  QResCursor cursor = robot_memory->query(fromjson("{plan:1}"), cfg_plan_collection_);
  while ( cursor->more() ) {
    BSONObj obj = cursor->next();
    std::vector<BSONElement> actions = obj.getField("actions").Array();
    for ( auto &a : actions ) {
      BSONObj o = a.Obj();
      std::string args;
      for ( auto &arg : o.getField("args").Array() ) {
        args += arg.str();
      }
      stn_->add_plan_action(o.getField("name").str(), args);
    }
  }
  logger->log_info(name(), "STN Generation finished.");
}

void
StnGeneratorThread::finalize()
{
  delete stn_;
}

void
StnGeneratorThread::bb_interface_data_changed(Interface *interface) throw()
{
  if ( interface->uid() == plan_if_->uid() ) {
    plan_if_->read();
    if ( plan_if_->is_final() ) {
      logger->log_info(name(), "Planning is final, starting STN generation");
      wakeup();
    }
  } else {
    logger->log_error(name(), "Received data change for wrong interface");
  }
}
