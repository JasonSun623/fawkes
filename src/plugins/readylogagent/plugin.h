
/***************************************************************************
 *  readylog_agent.h - Fawkes Readylog Agent Plugin
 *
 *  Created: Wed Jul 15 10:46:31 2009
 *  Copyright  2009  Daniel Beck
 *
 *  $Id: worldmodel_plugin.h 1462 2008-10-27 15:02:00Z tim $
 *
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

#ifndef __PLUGINS_READYLOG_AGENT_PLUGIN_H_
#define __PLUGINS_READYLOG_AGENT_PLUGIN_H_

#include <core/plugin.h>

class ReadylogAgentPlugin : public fawkes::Plugin
{
 public:
  ReadylogAgentPlugin( fawkes::Configuration* config );
};

#endif /* __PLUGINS_READYLOG_AGENT_PLUGIN_H_ */