
/***************************************************************************
 *  decrypt.h - WorldInfo decryption routine
 *
 *  Created: Thu May 03 15:53:20 2007
 *  Copyright  2006-2007  Tim Niemueller [www.niemueller.de]
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

#ifndef __NETCOMM_WORLDINFO_DECRYPT_H_
#define __NETCOMM_WORLDINFO_DECRYPT_H_

#include <core/exception.h>
#include <cstddef>

class MessageDecryptionException : public Exception
{
 public:
  MessageDecryptionException(char *msg);
};


class WorldInfoMessageDecryptor
{
 public:
  WorldInfoMessageDecryptor(const unsigned char *key, const unsigned char *iv);
  ~WorldInfoMessageDecryptor();

  void set_plain_buffer(void *buffer, size_t buffer_length);
  void set_crypt_buffer(void *buffer, size_t buffer_length);

  size_t decrypt();

 private:
  void    *plain_buffer;
  size_t   plain_buffer_length;
  void    *crypt_buffer;
  size_t   crypt_buffer_length;

  const unsigned char *key;
  const unsigned char *iv;
};


#endif
