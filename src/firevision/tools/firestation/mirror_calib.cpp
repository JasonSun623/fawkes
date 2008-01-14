
/***************************************************************************
 *  mirror_calib.cpp - Mirror calibration tool
 *
 *  Created: Fri Dec 07 18:35:40 2007
 *  Copyright  2007  Daniel Beck
 *
 *  $Id$
 *
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1307, USA.
 */

#include <tools/firestation/mirror_calib.h>
#include <models/mirror/bulb.h>
#include <models/mirror/bulb/bulb_sampler.h>
#include <core/exception.h>
#include <utils/math/angle.h>
#include <geometry/point.h>

/** @class MirrorCalibTool tools/firestation/mirror_calib.h
 * This class encapsulates the routines necessary for interactive mirror
 * calibration.
 */

using namespace std;

float MirrorCalibTool::m_sample_dist[] = {0.5, 1.0, 2.0, 3.0, 4.0};
float MirrorCalibTool::m_sample_ori[]  = {0.0, deg2rad(45.0),
					  deg2rad(90.0), deg2rad(135.0),
					  deg2rad(180.0),deg2rad(225.0),
					  deg2rad(270.0), deg2rad(315.0)};

/** Constructor. */
MirrorCalibTool::MirrorCalibTool()
{
  m_bulb = 0;
  m_sampler = 0;
  m_generator = 0;
  
  m_img_width = 0;
  m_img_height = 0;

  m_num_dists = 5 /*num_dists*/;
  m_num_oris = 8 /*num_oris*/;

  m_sample_step = 0;
  m_sample_dist_step = 0;
  m_sample_ori_step = 0;

  m_calib_done = false;
}

/** Constructor.
 * @param width image width
 * @param height image height
 */
MirrorCalibTool::MirrorCalibTool(unsigned int width, unsigned int height
				 /*, unsigned int num_dists, unsigned int num_oris*/ )
{
  m_bulb = 0;
  m_sampler = 0;
  m_generator = 0;

  m_img_width = width;
  m_img_height = height;

  m_num_dists = 5/*num_dists*/;
  m_num_oris = 8/*num_oris*/;

  m_sample_step = 0;
  m_sample_dist_step = 0;
  m_sample_ori_step = 0;

  m_calib_done = false;
}

/** Destructor. */
MirrorCalibTool::~MirrorCalibTool()
{
  delete m_bulb;
  delete m_sampler;
  delete m_generator;
}

/** Inititates the calibration process. */
void
MirrorCalibTool::start()
{
  m_sample_step = 0;
  m_sample_dist_step = 0;
  m_sample_ori_step = 0;

  m_step_two = false;

  m_sampler = new BulbSampler(m_img_width, m_img_height);
  m_next_sample_point = Point(0.0, 0.0, 0.0);
}

/** Aborts the calibration process. */
void
MirrorCalibTool::abort()
{
  m_sample_step = 0;
  m_sample_dist_step = 0;
  m_sample_ori_step = 0;
}

/** Do one step in the calibration process.
 * @param x the x-coordinate of the of the current calibration point 
 * @param y the y-coordinate of the of the current calibration point 
 */
void
MirrorCalibTool::step(unsigned int x, unsigned int y)
{
  if (m_sample_step == 0)
    {
      m_sampler->setCenter(x, y);
      cout << "Center set to (" << x << ", " << y << ")" << endl;
      cout << "Define forward direction" << endl;
      m_next_sample_point = Point(1.0, 0.0, 0.0);
      m_sample_step++;
    }
  else if (m_sample_step == 1)
    {
      m_sampler->calculateOmniOrientation(x, y);
      cout << "Forward pixel (" << x << ", " << y << ")" << endl;
      float dist = m_sample_dist[m_sample_dist_step];
      float ori = m_sample_ori[m_sample_ori_step];
      m_next_sample_point = Point(dist, 0.0, 0.0);
      m_next_sample_point.rotate_z(ori);
      m_step_two = true;
      m_sample_step++;
    }
  else if (m_sample_step == 2)
    {
      if (m_sample_ori_step < m_num_oris)
	{
	  if (m_sample_dist_step < m_num_dists)
	    {
	      m_sampler->setBallPosition(m_next_sample_point.x(), m_next_sample_point.y());
	      try
		{
		  m_sampler->consider(x, y, 0.0, 0.0, 0.0);
		}
	      catch (Exception &e)
		{
		  e.print_trace();
		}

	      m_sample_dist_step++;
	      if (m_sample_dist_step == m_num_dists)
		{
		  m_sample_ori_step++;
		  m_sample_dist_step = 0;
		}

	      if (m_sample_ori_step < m_num_oris)
		{
		  float dist = m_sample_dist[m_sample_dist_step];
		  float ori = m_sample_ori[m_sample_ori_step];
		  cout << m_sample_ori_step << ":" << m_sample_dist_step 
		       << ": Calibrating relative position (" 
		       << dist << ", " << ori << ")" << endl;
		  m_next_sample_point = Point(dist, 0.0, 0.0);
		  m_next_sample_point.rotate_z(ori);
		  cout << "Next sampling point (" << m_next_sample_point.x() << ", " << m_next_sample_point.y() << ")" << endl;
		}
	      else
		{
		  m_step_two = false;
		}
	    }
	}
      else
	{
	  m_sample_step++;
	}
    }
  else if (m_sample_step == 3)
    {
      cout << "Generating bulb" << endl;
      m_generator = new BulbGenerator(m_sampler, this);
      m_generator->generate();
      m_calib_done = true;
      cout << "Calibration done" << endl;
    }
}

/** Returns the world coordinates of the next calibration point in polar
 * coordinates.
 * @param dist distance to the next calibration point
 * @param ori relative orientation towards the next calibration point
 * @return true if the next calibration point can be determined
 */
bool
MirrorCalibTool::get_next(float* dist, float* ori)
{
  if (m_step_two)
    {
      *dist = m_sample_dist[m_sample_dist_step];
      *ori = rad2deg(m_sample_ori[m_sample_ori_step]);
      
      return true;
    }
  else
    {
      *dist = 0;
      *ori = 0;
      return false;
    }
} 

/** Loads a calibration file.
 * @param filename name of the file containing the calibration data
 */
void
MirrorCalibTool::load(const char* filename)
{
  m_bulb = new Bulb(filename);
}

/** Saves calibration data to a file.
 * @param filename the nem of the file
 */
void
MirrorCalibTool::save(const char* filename)
{
  if (m_calib_done)
    {
      m_generator->getResult()->save(filename);
    }
  else
    {
      cout << "Can't save in the middle of the calibration" << endl;
    }
}

/** Determines the world coordinates for the given image point.
 * @param x x-coordinate of the image point
 * @param y y-coordinate of the image point
 * @param dist_ret pointer to the relative distance of the world point
 * @param ori_ret pointer to the relative orientation of the world point
 */
void
MirrorCalibTool::eval(unsigned int x, unsigned int y, float* dist_ret, float* ori_ret)
{
  polar_coord_t coord;
  coord = m_bulb->getWorldPointRelative(x, y);

  *dist_ret = coord.r;
  *ori_ret = coord.phi;
}

/** Setter routine for the image dimensions.
 * @param width image width
 * @param height image height
 */
void
MirrorCalibTool::set_img_dimensions(unsigned int width, unsigned int height)
{
  m_img_width = width;
  m_img_height = height;
}

/** Setter routine for the distances used for calibration.
 * @param dists array of distances
 * @param num_dists number of distances
 */
void
MirrorCalibTool::set_dists(float dists[], unsigned int num_dists)
{
  // TODO
}

/** Setter routine for the orientations used for calibration.
 * @param oris array of orientations
 * @param num_oris number of orientations
 */
void
MirrorCalibTool::set_oris(float oris[], unsigned int num_oris)
{
  // TODO
}

void
MirrorCalibTool::setTotalSteps(unsigned int total_steps)
{
}

void
MirrorCalibTool::setProgress(unsigned int progress)
{
}

void
MirrorCalibTool::finished()
{
}