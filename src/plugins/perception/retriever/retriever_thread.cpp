
/***************************************************************************
 *  retriever_thread.cpp - FireVision Retriever Thread
 *
 *  Created: Tue Jun 26 17:39:11 2007
 *  Copyright  2006-2008  Tim Niemueller [www.niemueller.de]
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

#include "retriever_thread.h"

#include <fvcams/camera.h>
#include <fvmodels/color/lookuptable.h>
#include <fvutils/ipc/shm_image.h>
#include <fvutils/writers/jpeg.h>
#include <fvutils/writers/seq_writer.h>
#include <utils/time/tracker.h>

#include <cstdlib>
#include <cstring>

using namespace fawkes;
using namespace firevision;

/** @class FvRetrieverThread "retriever_thread.h"
 * FireVision retriever thread.
 * This implements the functionality of the FvRetrieverPlugin.
 * @author Tim Niemueller
 */

/** Constructor.
 * @param camera_string camera argument string for camera to open
 * @param cfg_name ID used to form thread name (FvRetrieverThread_[ID]) and shared
 * memory image segment ID (retriever_[ID]).
 * @param cfg_prefix configuration prefix path
 */
FvRetrieverThread::FvRetrieverThread(const std::string &camera_string,
                                     const std::string &cfg_name,
                                     const std::string &cfg_prefix)
: Thread("FvRetrieverThread", Thread::OPMODE_WAITFORWAKEUP),
  VisionAspect(VisionAspect::CYCLIC),
  cfg_name_(cfg_name),
  cfg_prefix_(cfg_prefix),
  camera_string_(camera_string),
  cap_time_(NULL)
{
	set_name("FvRetrieverThread_%s", cfg_name_.c_str());
	seq_writer = NULL;
}

/** Destructor. */
FvRetrieverThread::~FvRetrieverThread()
{
}

void
FvRetrieverThread::init()
{
	colorspace_t cspace     = YUV422_PLANAR;
	std::string  cspace_str = colorspace_to_string(cspace);
	try {
		cspace_str = config->get_string((cfg_prefix_ + "colorspace").c_str());
		cspace     = colorspace_by_name(cspace_str.c_str());
	} catch (Exception &e) {
	} // ignored, use default
	if (cspace == CS_UNKNOWN) {
		throw Exception("Unknown colorspace '%s' configured", cspace_str.c_str());
	}

	try {
		logger->log_debug(name(),
		                  "Registering for camera '%s' (colorspace %s)",
		                  camera_string_.c_str(),
		                  colorspace_to_string(cspace));
		cam = vision_master->register_for_camera(camera_string_.c_str(), this, cspace);
	} catch (Exception &e) {
		e.append("FvRetrieverThread::init() failed");
		throw;
	}
	try {
		char *imgbufname;
		if (asprintf(&imgbufname, "retriever_%s", cfg_name_.c_str()) == -1) {
			throw Exception("Cannot allocate buffer name");
		}
		shm = new SharedMemoryImageBuffer(imgbufname,
		                                  cam->colorspace(),
		                                  cam->pixel_width(),
		                                  cam->pixel_height());

		free(imgbufname);
		if (!shm->is_valid()) {
			throw Exception("Shared memory segment not valid");
		}
	} catch (Exception &e) {
		delete cam;
		cam = NULL;
		throw;
	}

	try {
		std::string frame_id = config->get_string((cfg_prefix_ + "frame").c_str());
		shm->set_frame_id(frame_id.c_str());
	} catch (Exception &e) { /* ignored, not critical */
	}

	seq_writer = NULL;
	try {
		if (config->get_bool("/firevision/retriever/save_images")) {
			logger->log_info(name(), "Writing images to disk");
			Writer *writer = new JpegWriter();
			seq_writer     = new SeqWriter(writer);
			std::string save_path;
			try {
				save_path = config->get_string("/firevision/retriever/save_path");
			} catch (Exception &e) {
				save_path = ("recorded_images");
				logger->log_info(name(), "No save path specified. Using './%s'", save_path.c_str());
			}
			seq_writer->set_path(save_path.c_str());
			seq_writer->set_dimensions(cam->pixel_width(), cam->pixel_height());
			seq_writer->set_colorspace(cam->colorspace());
		}
	} catch (Exception &e) {
		// ignored, not critical
	}

	tt_ = NULL;
	try {
		if (config->get_bool("/firevision/retriever/use_time_tracker")) {
			tt_          = new TimeTracker();
			ttc_capture_ = tt_->add_class("Capture");
			ttc_memcpy_  = tt_->add_class("Memcpy");
			ttc_dispose_ = tt_->add_class("Dispose");
			loop_count_  = 0;
		}
	} catch (Exception &e) {
		// ignored, not critical
	}

	cm_              = new ColorModelLookupTable(1, "retriever-colormap", true);
	YuvColormap *ycm = cm_->get_colormap();
	for (unsigned int u = 100; u < 150; ++u) {
		for (unsigned int v = 100; v < 150; ++v) {
			ycm->set(128, u, v, C_ORANGE);
		}
	}

	cam_has_timestamp_support_ = true;
	try {
		fawkes::Time *t = cam->capture_time();
		if (t->is_zero()) {
			throw NotImplementedException("");
		}
		cap_time_ = NULL;
	} catch (NotImplementedException &e) {
		cam_has_timestamp_support_ = false;
		cap_time_                  = new Time(clock);
	}
}

void
FvRetrieverThread::finalize()
{
	logger->log_debug(name(), "Unregistering from vision master");
	vision_master->unregister_thread(this);
	delete cam;
	delete shm;
	delete seq_writer;
	delete tt_;
	delete cm_;
	delete cap_time_;
}

/** Thread loop. */
void
FvRetrieverThread::loop()
{
	if (tt_) {
		// use time tracker
		tt_->ping_start(ttc_capture_);
		cam->capture();
		tt_->ping_end(ttc_capture_);
		tt_->ping_start(ttc_memcpy_);
		shm->lock_for_write();
		memcpy(shm->buffer(), cam->buffer(), cam->buffer_size() - 1);
		shm->unlock();
		tt_->ping_end(ttc_memcpy_);
		if (cam_has_timestamp_support_)
			shm->set_capture_time(cam->capture_time());
		tt_->ping_start(ttc_dispose_);
		cam->dispose_buffer();
		tt_->ping_end(ttc_dispose_);
		if ((++loop_count_ % 200) == 0) {
			// output results every 200 loops
			tt_->print_to_stdout();
		}
	} else {
		// no time tracker
		cam->capture();
		shm->lock_for_write();
		memcpy(shm->buffer(), cam->buffer(), cam->buffer_size());
		shm->unlock();
		if (cam_has_timestamp_support_) {
			shm->set_capture_time(cam->capture_time());
		} else {
			cap_time_->stamp();
			shm->set_capture_time(cap_time_);
		}
		cam->dispose_buffer();
	}

	if (seq_writer) {
		seq_writer->write(shm->buffer());
	}
}
