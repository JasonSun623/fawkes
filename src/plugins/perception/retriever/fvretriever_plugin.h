
/***************************************************************************
 *  fvretriever_plugin.h - FireVision Retriever Plugin
 *
 *  Created: Tue Jun 26 17:34:51 2007
 *  Copyright  2006-2007  Tim Niemueller [www.niemueller.de]
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

#ifndef _FIREVISION_APPS_RETRIEVER_FVRETRIEVER_PLUGIN_H_
#define _FIREVISION_APPS_RETRIEVER_FVRETRIEVER_PLUGIN_H_

#include <core/plugin.h>

class FvRetrieverPlugin : public fawkes::Plugin
{
public:
	explicit FvRetrieverPlugin(fawkes::Configuration *config);
};

#endif
