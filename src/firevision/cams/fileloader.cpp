
/***************************************************************************
 *  fileloader.cpp - A camera which obtains its images from a single image
 *                   file or from several image files in a directory
 *
 *  Generated: Tue Feb 22 13:28:08 2005
 *  Copyright  2005-2007  Tim Niemueller [www.niemueller.de]
 *             2008       Daniel Beck
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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <core/exception.h>
#include <core/exceptions/software.h>
#include <cams/fileloader.h>
#include <fvutils/writers/fvraw.h>
#include <fvutils/system/filetype.h>
#include <fvutils/system/camargp.h>

#include <fvutils/readers/fvraw.h>
#ifdef HAVE_LIBJPEG
#include <fvutils/readers/jpeg.h>
#endif

#include <cstring>
#include <cstdlib>
#include <cstdio>

#include <sys/types.h>
#include <dirent.h>

/** @class FileLoader <cams/fileloader.h>
 * Load images from files.
 * The file loader tries to determine the image format of the given image using
 * the the file type utility. Currently it recognizes JPEG and FvRaw image files.
 *
 * @author Tim Niemueller
 * @author Daniel Beck
 */

char* FileLoader::extension = NULL;

int file_select(const struct dirent* ent)
{
  if ( !FileLoader::extension ) { return 1; }

  // NOTE: this only checks whether the filename contains the
  // extension and not whether it ends with it.
  if ( NULL != strstr(ent->d_name, FileLoader::extension) ) {
    return 1; 
  }

  return 0;
}

/** Constructor.
 * @param filename name of file to open, full path or relative to working directory
 */
FileLoader::FileLoader(const char *filename)
{
  this->filename = strdup(filename);
  this->dirname = NULL;
  this->extension = NULL;
  this->file_list = NULL;
  num_files = 0;
  cur_file = 0;
  opened = started = false;
  width = height = 0;
  file_buffer = NULL;
  this->cspace = CS_UNKNOWN;
}


/** Constructor.
 * Initialize with the parameters from the given camera argument parser. The following
 * parameters are supported:
 * - file=FILENAME: open the given file
 * - dir=DIRECTORY: sequentially open files in this directory
 * - ext=EXTENSION: only open files with this extension
 *
 * @param cap camera argument parser
 */
FileLoader::FileLoader(const CameraArgumentParser *cap)
{
  filename = NULL;
  dirname = NULL;

  if ( cap->has("file") ) {
    this->filename = strdup(cap->get("file").c_str());
  } else if ( cap->has("dir") ) {
    this->dirname = strdup( cap->get("dir").c_str() );
    if ( cap->has("ext") ) {
      this->extension = strdup( cap->get("ext").c_str() );
    }
  } else {
    throw MissingParameterException("Neither parameter file nor parameter directory are present");
  }

  file_list = NULL;
  num_files = 0;
  cur_file = 0;
  width = height = 0;
  file_buffer = NULL;
  this->cspace = CS_UNKNOWN;
  opened = started = false;
}


/** Legacy constructor.
 * Before FvRaw FireVision had the ability to store the pure buffer of an image
 * without any header. Because of this additional information like colorspace,
 * width and height of the image had to be supplied. The number of bytes that
 * has to be read for the image is calculated from the given parameters.
 * @param cspace color space of image
 * @param filename filename to open
 * @param width width of image
 * @param height height of image
 */
FileLoader::FileLoader(colorspace_t cspace, const char *filename,
		       unsigned int width, unsigned int height)
{
  started = opened = false;
  this->cspace = cspace;
  this->width = width;
  this->height = height;
  this->filename = strdup(filename);
  this->dirname = NULL;
  this->extension = NULL;
  this->file_list = NULL;
  num_files = 0;
  cur_file = 0;
  file_buffer = NULL;
}


/** Destructor. */
FileLoader::~FileLoader()
{
  for (int i = 0; i < num_files; ++i) {
    free(file_list[i]);
  }
  free(file_list);
  free(dirname);
  free(extension);
  free(filename);
}


void
FileLoader::open()
{
  if (opened) return;

  if (dirname) {
    num_files = scandir(dirname, &file_list, file_select, alphasort);
    
    if ( -1 == num_files ) {
      throw Exception("Error while scanning directory %s", dirname);
    }
  }

  read_file();
  opened = true;
}


void
FileLoader::start()
{
  if (started) return;

  if (!opened) {
    throw Exception("Trying to start closed file");
  }

  started = true;
}

void
FileLoader::stop()
{
  started = false;
}


void
FileLoader::print_info()
{
}


void
FileLoader::capture()
{
  if (0 != num_files) {
    if (file_buffer) {
      free(file_buffer);
    }

    read_file();

    if (++cur_file == num_files) {
      cur_file = 0;
    }
  }
}


unsigned char*
FileLoader::buffer()
{
  return file_buffer;
}


unsigned int
FileLoader::buffer_size()
{
  return _buffer_size;
}


void
FileLoader::close()
{
  if (file_buffer != NULL) {
    free(file_buffer);
    file_buffer = NULL;
  }
  opened = false;
}


void
FileLoader::dispose_buffer()
{
}


void
FileLoader::flush()
{
}


bool
FileLoader::ready()
{
  return started;
}


void
FileLoader::set_image_number(unsigned int n)
{
}


unsigned int
FileLoader::pixel_width()
{
  return width;
}


unsigned int
FileLoader::pixel_height()
{
  return height;
}


colorspace_t
FileLoader::colorspace()
{
  return cspace;
}


/** Set the colorspace of the image.
 * @param c colorspace
 */
void
FileLoader::set_colorspace(colorspace_t c)
{
  cspace = c;
}


/** Set width.
 * @param w image width in pixels
 */
void
FileLoader::set_pixel_width(unsigned int w)
{
  width = w;
}


/** Set height.
 * @param h image height in pixels
 */
void
FileLoader::set_pixel_height(unsigned int h)
{
  height = h;
}

void
FileLoader::read_file()
{
  char* fn;
  if (0 != num_files) {
    asprintf(&fn, "%s/%s", dirname, file_list[cur_file]->d_name);
  } else {
    fn = strdup(filename);
  }

  std::string ft = fv_filetype_file( fn );

  if ( FvRawReader::is_FvRaw(fn) ) {
    FvRawReader *fvrr = new FvRawReader( fn );
    cspace = fvrr->colorspace();
    width  = fvrr->pixel_width();
    height = fvrr->pixel_height();
    _buffer_size = colorspace_buffer_size( cspace, width, height );
    file_buffer = (unsigned char*)malloc(_buffer_size);
    fvrr->set_buffer( file_buffer );
    try {
      fvrr->read();
    } catch (Exception &e) {
      delete fvrr;
      e.append("FileLoader::open() failed");
      throw;
    }
    delete fvrr;

#ifdef HAVE_LIBJPEG
  } else if ( ft.find( "JPEG" ) != std::string::npos ) {
    JpegReader *jr = new JpegReader( fn );
    cspace = jr->colorspace();
    width  = jr->pixel_width();
    height = jr->pixel_height();
    _buffer_size = colorspace_buffer_size( cspace, width, height );
    file_buffer = (unsigned char*)malloc(_buffer_size);
    jr->set_buffer( file_buffer );
    try {
      jr->read();
    } catch (Exception &e) {
      delete jr;
      e.append("FileLoader::open() failed");
      throw;
    }
    delete jr;
#endif

  } else {
    _buffer_size = colorspace_buffer_size( cspace, width, height );

    if (_buffer_size > 0) {
      FILE *f;
      f = fopen( fn, "rb" );
      file_buffer = (unsigned char*)malloc(_buffer_size);
      if (fread(file_buffer, _buffer_size, 1, f) != 1) {
	// cout << "FileLoader: Could not read data." << endl;
	fclose(f);
	throw Exception("Could not read data");
      }
      fclose(f);
    } else {
      throw Exception("Invalid color space (buffer size is 0)");
    }
  }

  free(fn);
}

