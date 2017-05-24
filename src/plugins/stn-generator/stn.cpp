 // only if type is Expression
/***************************************************************************
 *  stn.cpp - stn-generator
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
#include <iostream>
#include "pddl_parser.h"

#include "stn.h"

namespace fawkes {
namespace stn {

Stn::Stn(fawkes::Logger* logger)
{
  logger_ = logger;
}

Stn::Stn(fawkes::Logger* logger, std::string classic_dom_path)
{
  logger_ = logger;
  classic_dom_path_ = classic_dom_path;
  gen_classic_dom_ = true;
}

Stn::~Stn()
{
}

void
Stn::add_domain_action(DomainAction action)
{
  domain_actions_.push_back(action);
}

void
Stn::add_plan_action(std::string name, std::string params)
{
  plan_actions_.push_back(plan_action{name, params});
}

void
Stn::set_initial_state(StnAction action)
{
  initial_state_ = action;
}

void
Stn::set_pddl_domain(std::string pddl_domain_string)
{
  pddl_parser::PddlParser parser;
  pddl_parser::Domain dom = parser.parseDomain(pddl_domain_string);

  log_info("Loading extended PDDL domain into STN.");

  for ( auto& action : dom.actions) {
    log_info("Processing action " + action.name);
    std::vector<std::string> params;
    for ( auto& param : action.action_params ) {
      params.push_back(param.first);
    }
    std::vector<Predicate> preconds;
    build_pred_list(action.precondition, &preconds, true);
    std::vector<Predicate> effects;
    build_pred_list(action.effect, &effects, true);
    int duration = action.duration;
    std::vector<std::string> cond_breakups;
    log_info(std::to_string(action.cond_breakup.which()));
    if ( action.cond_breakup.which() == 1 ) { // only if type is Expression
      build_breakup_list(action.cond_breakup, &cond_breakups);
    }
    std::vector<std::string> temp_breakups;
    if ( action.temp_breakup.which() == 1 ) { // only if type is Expression
      build_breakup_list(action.temp_breakup, &temp_breakups);
    }
    DomainAction da(action.name, params, preconds, effects, duration, cond_breakups, temp_breakups);
    domain_actions_.push_back(da);
    //TODO remove after plugin is fully working
    std::stringstream ss;
    ss << da;
    log_info("Added action:\n" + ss.str());
  }

  log_info("Initialized " + std::to_string(domain_actions_.size()) +
      " domain actions");

  if ( gen_classic_dom_ ) {
    log_info("Generation of classic domain file is configured, starting...");
    generate_classic_pddl_domain(&dom, classic_dom_path_);
  }
}

/* For now this only works with the not and and operators
 * to combine multiple predicates
 */
void
Stn::build_pred_list(pddl_parser::Expression e,
    std::vector<Predicate> *preconds, bool condition)
{
  pddl_parser::Atom function = boost::get<pddl_parser::Predicate>(e).function;
  if ( function == "and" || function == "not" ) {
    if ( function == "not" ) {
      condition = !condition;
    }
    for ( auto& child : boost::get<pddl_parser::Predicate>(e).arguments ) {
      build_pred_list(child, preconds, condition);
    }
  } else {
    std::vector<std::string> args;
    for ( auto& arg : boost::get<pddl_parser::Predicate>(e).arguments ) {
      args.push_back(boost::get<std::string>(arg));
    }
    Predicate p(boost::get<pddl_parser::Predicate>(e).function, condition, args);
    preconds->push_back(p);
    log_info("Added " + boost::get<pddl_parser::Predicate>(e).function);
  }
}

void
Stn::build_breakup_list(pddl_parser::Expression e,
    std::vector<std::string> *breakups)
{
  pddl_parser::Atom function = boost::get<pddl_parser::Predicate>(e).function;
  // ignore negations, we only take the name into account
  if ( function == "and" || function == "not" ) {
    for ( auto& child : boost::get<pddl_parser::Predicate>(e).arguments ) {
      build_breakup_list(child, breakups);
    }
  } else {
    std::string pred_name = boost::get<pddl_parser::Predicate>(e).function;
    breakups->push_back(pred_name);
  }
}

void
Stn::generate()
{
  stn_actions_.clear();
  //stn_actions_.push_back(initial_state_);

  for ( plan_action pa : plan_actions_ ) {
    std::vector<DomainAction>::iterator it = domain_actions_.begin();
    for ( ; it != domain_actions_.end(); it++ ) {
      if ( it->getName() == pa.name ) {
        break;
      }
    }
    if ( it == domain_actions_.end() ) throw("could not find fitting DomainAction");
    DomainAction da = *(it);

    stn_actions_.push_back(da.generateStnAction(pa.name, pa.params));
  }
  std::cout << "Imported " << stn_actions_.size() << " actions into STN" << std::endl;
  
  for ( int i = stn_actions_.size() - 1; i >= 0; i-- ) {
    std::vector<StnAction> candidate_actions = std::vector<StnAction>(
        stn_actions_.begin(), stn_actions_.begin()+i);
    stn_actions_.at(i).genConditionalActions(candidate_actions);
  }

  std::vector<Predicate> predicates;
  for ( std::vector<StnAction>::iterator it = stn_actions_.begin(); it != stn_actions_.end(); it++ ) {
    // add conditional edges
    for ( auto const &cond_action : it->condActionIds()) {
      std::pair<StnAction,StnAction> edge(findActionById(cond_action), findActionById(it->id()));
      cond_edges_.push_back(edge);
    }
    // add temporal edges
    bool break_edge = false;
    for ( Predicate p : predicates ) {
      if ( it->checkForBreakup(EdgeType::TEMPORAL, p) ) {
        std::cout << "Break because of: " << p;
        break_edge = true;
        break;
      }
    }
    if ( !break_edge && it != stn_actions_.begin() ) {
      std::pair<StnAction,StnAction> edge(findActionById((it - 1)->id()), findActionById(it->id()));
      temp_edges_.push_back(edge);
    }
    // handle predicates
    for ( Predicate p : it->effects() ) {
      if ( p.condition() ) {
        std::vector<Predicate>::iterator it = std::find(predicates.begin(), predicates.end(), p);
        if ( it == predicates.end() ) {
          predicates.push_back(p);
          //std::cout << "Added " << p;
        }
      } else {
        //std::cout << "Check for erase: " << p;
        Predicate neg_pred(p.name(), true, p.attrs() );
        std::vector<Predicate>::iterator it = std::find(predicates.begin(), predicates.end(), neg_pred);
        if ( it != predicates.end() ) {
          //std::cout << "Erased " << (*it);
          predicates.erase(it);
        }
      }
    }
  }
  //generate missing temporal links
  for ( auto &a : stn_actions_ ) {
    bool no_temp_edge = true;
    for ( auto &e : temp_edges_ ) {
      if ( e.first == a ) {
        no_temp_edge = false;
        break;
      }
    }
    if ( no_temp_edge ) {
      for ( auto &ce : cond_edges_ ) {
        if ( ce.first == a ) {
          std::pair<StnAction,StnAction> edge(a, ce.second);
          temp_edges_.push_back(edge);
        }
      }
    }
  }
}

void
Stn::drawGraph()
{
  Agraph_t* G;
  GVC_t* gvc;

  gvc = gvContext();
  char graph_name[] = "STN";

  G = agopen(graph_name,Agdirected,0);

  std::map<size_t, Agnode_t *> node_map;

  for ( StnAction a : stn_actions_ ) {
    std::string node_name = a.genGraphNodeName();
    node_map.insert( std::make_pair(
          a.id(), agnode(G, (char*)node_name.c_str(), true)));
  }

  std::vector<Agedge_t *> edge_list;
  for ( auto& edge : cond_edges_ ) {
    Agnode_t * node1 = node_map.at(edge.first.id());
    Agnode_t * node2 = node_map.at(edge.second.id());
    Agedge_t *graph_edge = agedge(G, node1, node2, (char*)"conditional", true);
    edge_list.push_back(graph_edge);

    std::string edge_label = edge.second.genConditionEdgeLabel(edge.first.id());
    agsafeset (graph_edge, (char*)"label", agstrdup_html(G,(char*)edge_label.c_str()), (char*)"");
    agsafeset (graph_edge, (char*)"color",(char*)"red", (char*)"");
  }

  for ( auto& edge : temp_edges_ ) {
    Agnode_t * node1 = node_map.at(edge.first.id());
    Agnode_t * node2 = node_map.at(edge.second.id());
    Agedge_t *graph_edge = agedge(G, node1, node2, (char*)"temporal", true);
    edge_list.push_back(graph_edge);

    std::string edge_label = edge.second.genTemporalEdgeLabel();
    agsafeset (graph_edge, (char*)"label", agstrdup_html(G,(char*)edge_label.c_str()), (char*)"");
    agsafeset (graph_edge, (char*)"color",(char*)"blue", (char*)"");
  }

  gvLayout(gvc,G,"dot");
  gvRenderFilename(gvc,G,"png","stn.png");

  gvFreeLayout(gvc, G);
  agclose (G);
  gvFreeContext(gvc);


}

StnAction
Stn::findActionById(size_t id)
{
  for ( StnAction a : stn_actions_ ) {
    if ( a.id() == id ) {
      return a;
    }
  }
  throw (" Action with id " + std::to_string(id) + " not found");
}

void
Stn::log_warn(std::string s)
{
  log(s, LogLevel::WARN);
}

void
Stn::log_info(std::string s)
{
  log(s, LogLevel::INFO);
}

void
Stn::log_debug(std::string s)
{
  log(s, LogLevel::DEBUG);
}
void
Stn::log(std::string s, Stn::LogLevel log_level)
{
  std::string name = "STN";
  switch (log_level) {
    case LogLevel::WARN:
      logger_->log_warn(name.c_str(), s.c_str());
      break;
    case LogLevel::INFO:
      logger_->log_info(name.c_str(), s.c_str());
      break;
    case LogLevel::DEBUG:
      logger_->log_debug(name.c_str(), s.c_str());
      break;
  }
}

void
Stn::generate_classic_pddl_domain(pddl_parser::Domain *dom,
    std::string classic_dom_path)
{
  log_info("Writing domain to " + classic_dom_path);
  std::ofstream out(classic_dom_path);
  
  out << "(define (domain " << dom->name << ")" << std::endl;
  
  out << "\t(:requirements";
  for ( auto& req : dom->requirements ) {
    out << " :" << req;
  }
  out << ")" << std::endl;
  
  out << "\t(:types" << std::endl;
  for ( auto& type : dom->types ) {
    out << "\t\t" << type.first << " - " << type.second << std::endl;
  }
  out << "\t)" << std::endl;

  out << "\t(:constants" << std::endl;
  for ( auto& const_type : dom->constants ) {
    out << "\t\t";
    for ( auto& constant : const_type.first ) {
      out << constant << " ";
    }
    out << "- " << const_type.second << std::endl;
  }
  out << "\t)" << std::endl;

  out << "\t(:predicates" << std::endl;
  for ( auto& predicate : dom->predicates ) {
    out << "\t\t(" << predicate.first;
    for ( auto& pred_type : predicate.second ) {
      out << " ?" << pred_type.first << " - " << pred_type.second;
    }
    out << ")" << std::endl;
  }
  out << "\t)" << std::endl;

  for ( auto& action: dom->actions ) {
    out << "\t(:action " << action.name << std::endl;
    out << "\t\t:parameters (";
    for ( auto& param: action.action_params ) {
      out << " ?" << param.first << " - " << param.second;
    }
    out << ")" << std::endl;
    out << "\t\t:precondition (and" << std::endl;
    std::vector<Predicate> preds;
    build_pred_list(action.precondition, &preds, true);
    for ( auto& pred : preds ) {
      out << "\t\t\t";
      if ( !pred.condition() ) {
        out << "(not ";
      }
      out << "(" << pred.name();
      for ( auto& a : pred.attrs() ) {
        out << " " << a;
      }
      if ( !pred.condition() ) {
        out << ")";
      }
      out << ")" << std::endl;
    }
    out << "\t\t)" << std::endl;
    out << "\t\t:effect (and" << std::endl;
    preds.clear();
    build_pred_list(action.effect, &preds, true);
    for ( auto& pred : preds ) {
      out << "\t\t\t";
      if ( !pred.condition() ) {
        out << "(not ";
      }
      out << "(" << pred.name();
      for ( auto& a : pred.attrs() ) {
        out << " " << a;
      }
      if ( !pred.condition() ) {
        out << ")";
      }
      out << ")" << std::endl;
    }
    out << "\t\t)" << std::endl;

    out << "\t)" << std::endl;

  }

  out << ")";

  out.close();
}

}
}
