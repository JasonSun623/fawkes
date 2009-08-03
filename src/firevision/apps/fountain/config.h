/***************************************************************************
 *  config.h - config helper - generated by genconfig 0.3
 *
 *  Config class generated: Mon Jun 18 14:57:15 2007
 *  Used template: ../../../../../src/modules/tools/genconfig/config.template.h
 *  Template created: Thu Apr 29 14:26:23 2004
 *  Copyright  2004  Tim Niemueller
 *  niemueller@i5.informatik.rwth-aachen.de
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
 
#ifndef __APPS_FOUNTAIN__RCSOFTXFOUNTAINCONFIG_H_
#define __APPS_FOUNTAIN__RCSOFTXFOUNTAINCONFIG_H_

/// @cond RCSOFTX_CONFIG

#include <utils/config_reader/config_reader.h>
#include <string>

class RCSoftXFountainConfig
{
 public:
  RCSoftXFountainConfig(CConfigReader *configFile);
  ~RCSoftXFountainConfig();

  /** FountainPort value */
  int FountainPort;
  /** ImageDelay value */
  int ImageDelay;


 protected:
  /** Config reader */
  CConfigReader * m_pXMLConfigFile;
  /** Current config prefix. */
  std::string m_sPrefix;

};

/// @endcond

#endif /* __APPS_FOUNTAIN__RCSOFTXFOUNTAINCONFIG_H_ */

