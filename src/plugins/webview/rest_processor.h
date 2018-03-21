
/***************************************************************************
 *  rest_processor.h - Web request processor for an extensible REST API
 *
 *  Created: Fri Mar 16 12:02:03 2018
 *  Copyright  2006-2018  Tim Niemueller [www.niemueller.de]
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

#ifndef __PLUGINS_WEBVIEW_REST_PROCESSOR_H_
#define __PLUGINS_WEBVIEW_REST_PROCESSOR_H_

#include <webview/request_processor.h>

#include <string>

namespace fawkes {
	class WebviewRestApiManager;
  class Logger;
}

class WebviewRESTRequestProcessor : public fawkes::WebRequestProcessor
{
 public:
  WebviewRESTRequestProcessor(const char *baseurl,
                              fawkes::WebviewRestApiManager *api_mgr,
                              fawkes::Logger *logger);
  virtual ~WebviewRESTRequestProcessor();

  virtual fawkes::WebReply * process_request(const fawkes::WebRequest *request);

 private:
  fawkes::WebviewRestApiManager *api_mgr_;
  fawkes::Logger *logger_;
  std::string     baseurl_;
};

#endif
