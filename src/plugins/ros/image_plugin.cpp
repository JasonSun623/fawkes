
/***************************************************************************
 *  image_plugin.cpp - Exchange point clouds between Fawkes and ROS
 *
 *  Created: Tue Apr 10 22:09:24 2012
 *  Copyright  2011-2012  Tim Niemueller [www.niemueller.de]
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

#include "image_thread.h"

#include <core/plugin.h>

using namespace fawkes;

/** Plugin exchange transforms between Fawkes and ROS.
 * @author Tim Niemueller
 */
class RosImagesPlugin : public fawkes::Plugin
{
public:
	/** Constructor.
   * @param config Fawkes configuration
   */
	explicit RosImagesPlugin(Configuration *config) : Plugin(config)
	{
		thread_list.push_back(new RosImagesThread());
	}
};

PLUGIN_DESCRIPTION("Export Fawkes images to ROS")
EXPORT_PLUGIN(RosImagesPlugin)
