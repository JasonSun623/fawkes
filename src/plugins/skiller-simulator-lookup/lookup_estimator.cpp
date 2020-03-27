/***************************************************************************
 *  lookup_estimator.cpp - Estimate skill exec times by random db lookups
 *
 *  Created: Tue 24 Jan 2020 11:25:31 CET 16:35
 *  Copyright  2020  Tarik Viehmann <viehmann@kbsg.rwth-aachen.de>
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

#include "lookup_estimator.h"

#include <aspect/logging.h>
#include <config/yaml.h>
#include <core/threading/mutex.h>
#include <core/threading/mutex_locker.h>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/exception/exception.hpp>
#include <chrono>
#include <mongocxx/client.hpp>
#include <mongocxx/exception/operation_exception.hpp>
#include <thread>

namespace fawkes {
namespace skiller_simulator {

/** @class LookupEstimator
 * Estimate the execution time of skills by drawing a random sample from a
 * set of possible values stored in a mongodb database.
 */

/** Constructor.
 * @param mongo_connection_manager The mongodb manager to connect to a lookup collection
 * @param config The config to retrieve database related info and the skills to estimates
 * @param logger The logger to inform about client connection status
 */
LookupEstimator::LookupEstimator(MongoDBConnCreator *mongo_connection_manager,
                                 Configuration *     config,
                                 Logger *            logger)
: mongo_connection_manager_(mongo_connection_manager), config_(config), logger_(logger)
{
	skills_ =
	  config_->get_strings_or_defaults((std::string(cfg_prefix_) + "/lookup/skills").c_str(), {});
	try_by_default_ =
	  config_->get_bool_or_default((std::string(cfg_prefix_) + "/try-by-default").c_str(), false);
	database_ =
	  config_->get_string_or_default((std::string(cfg_prefix_) + "/database").c_str(), "skills");
	collection_ = config_->get_string_or_default((std::string(cfg_prefix_) + "/collection").c_str(),
	                                             "exec_times");
	skill_name_field_ =
	  config_->get_string_or_default((std::string(cfg_prefix_) + "/skill-field").c_str(), "name");
	duration_field_ =
	  config_->get_string_or_default((std::string(cfg_prefix_) + "/duration-field").c_str(),
	                                 "duration");
	logger_->log_info(name_,
	                  ("Using database " + database_ + " collection " + collection_
	                   + " lookup fields: " + skill_name_field_ + " : " + duration_field_)
	                    .c_str());
}

void
LookupEstimator::init()
{
	unsigned int startup_grace_period =
	  config_->get_uint_or_default("/plugins/mongodb/instances/statistics-local/startup-grace-period",
	                               10);
	logger_->log_info(name_, "Connect to mongodb statistics-local instance");
	unsigned int startup_tries = 0;
	for (; startup_tries < startup_grace_period * 2; ++startup_tries) {
		try {
			mongodb_client_lookup_ = mongo_connection_manager_->create_client("statistics-local");
			logger_->log_info(name_, "Successfully connected to statistics-local instance");
			return;
		} catch (fawkes::Exception &) {
			using namespace std::chrono_literals;
			logger_->log_info(name_, "Waiting for mongodb statistics-local instance");
			std::this_thread::sleep_for(500ms);
		}
	}
	logger_->log_error(name_, "%s", "Failed to connect to mongodb statistics-local instance");
}

bool
LookupEstimator::can_execute(const Skill &skill)
{
	// if all skills should be looked up by default, then the skills_ contain
	// those skills that should not be estimated via lookup
	if (try_by_default_
	    ^ (std::find(skills_.begin(), skills_.end(), skill.skill_name) != skills_.end())) {
		MutexLocker lock(mutex_);
		std::string log_start = "survive the locker";
		logger_->log_error(name_, "%s", log_start.c_str());
		try {
			using bsoncxx::builder::basic::kvp;
			using bsoncxx::builder::basic::make_document;
			bsoncxx::stdx::optional<bsoncxx::document::value> found_entry =
			  mongodb_client_lookup_->database(database_)[collection_].find_one(
			    make_document(kvp(skill_name_field_, skill.skill_name)));
			return bool(found_entry);
		} catch (mongocxx::operation_exception &e) {
			std::string error =
			  std::string("Error trying to lookup " + skill.skill_name + "\n Exception: " + e.what());
			logger_->log_error(name_, "%s", error.c_str());
			return false;
		}
	} else {
		return false;
	}
}

float
LookupEstimator::get_execution_time(const Skill &skill)
{
	using bsoncxx::builder::basic::kvp;
	using bsoncxx::builder::basic::make_document;
	// pipeline to pick a random sample out of all documents with matching name
	// field
	mongocxx::pipeline p{};
	p.match(make_document(kvp(skill_name_field_, skill.skill_name)));
	p.sample(1);
	// lock as mongocxx::client is not thread-safe
	MutexLocker lock(mutex_);
	try {
		mongocxx::cursor sample_cursor =
		  mongodb_client_lookup_->database(database_)[collection_].aggregate(p);
		auto  doc = *(sample_cursor.begin());
		float res = 0.f;
		switch (doc[duration_field_].get_value().type()) {
		case bsoncxx::type::k_double: res = float(doc[duration_field_].get_double().value); break;
		case bsoncxx::type::k_int32: res = float(doc[duration_field_].get_int32().value); break;
		default:
			throw fawkes::Exception(("Unexpected type "
			                         + bsoncxx::to_string(doc[duration_field_].get_value().type())
			                         + " when looking up skill exec duration.")
			                          .c_str());
		}
		return res;
	} catch (mongocxx::operation_exception &e) {
		std::string error =
		  std::string("Error for lookup of " + skill.skill_name + "\n Exception: " + e.what());
		logger_->log_error(name_, "%s", error.c_str());
		return 1.f;
	}
}

void
LookupEstimator::execute(const Skill &skill)
{
}

} // namespace skiller_simulator
} // namespace fawkes