
/***************************************************************************
 *  fuse_server.tcp - network image transport server interface
 *
 *  Generated: Mon Mar 19 15:56:22 2007
 *  Copyright  2005-2007  Tim Niemueller [www.niemueller.de]
 *
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version. A runtime exception applies to
 *  this software (see LICENSE.GPL_WRE file mentioned below for details).
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL_WRE file in the doc directory.
 */

#include <fvutils/net/fuse_server.h>
#include <fvutils/net/fuse_server_client_thread.h>

#include <core/threading/thread_collector.h>
#include <netcomm/utils/acceptor_thread.h>

#include <algorithm>

using namespace fawkes;

/** @class FuseServer <fvutils/net/fuse_server.h>
 * FireVision FUSE protocol server.
 * The FuseServer will open a StreamSocket and listen on it for incoming
 * connections. For each connection a client thread is started that will process
 * all requests issued by the client.
 *
 * @ingroup FUSE
 * @ingroup FireVision
 * @author Tim Niemueller
 */

/** Constructor.
 * @param port Port to listen on for incoming connections
 * @param collector optional thread collector
 */
FuseServer::FuseServer(unsigned short int port, ThreadCollector *collector)
  : Thread("FuseServer", Thread::OPMODE_WAITFORWAKEUP)
{
  __thread_collector = collector;

  __acceptor_thread = new NetworkAcceptorThread(this, port, "FuseNetworkAcceptorThread");
  if (__thread_collector) {
    __thread_collector->add(__acceptor_thread);
  } else {
    __acceptor_thread->start();
  }
}


/** Destructor. */
FuseServer::~FuseServer()
{
  if ( __thread_collector ) {
    __thread_collector->remove(__acceptor_thread);
  } else {
    __acceptor_thread->cancel();
    __acceptor_thread->join();
  }

  for (__cit = __clients.begin(); __cit != __clients.end(); ++__cit) {
    if ( __thread_collector ) {
      // ThreadCollector::remove also stops the threads!
      __thread_collector->remove(*__cit);
    } else {
      (*__cit)->cancel();
      (*__cit)->join();
    }
    delete *__cit;
  }
  __clients.clear();

  delete __acceptor_thread;
}


void
FuseServer::add_connection(StreamSocket *s) throw()
{
  FuseServerClientThread *client = new FuseServerClientThread(this, s);
  if ( __thread_collector) {
    __thread_collector->add(client);
  } else {
    client->start();
  }
  __clients.push_back_locked(client);
}


/** Connection died.
 * @param client client whose connection died
 */
void
FuseServer::connection_died(FuseServerClientThread *client) throw()
{
  __dead_clients.push_back_locked(client);
  wakeup();
}


void
FuseServer::loop()
{
  // Check for dead clients, cancel and join if there are any
  __dead_clients.lock();
  __clients.lock();

  LockList<FuseServerClientThread *>::iterator  dcit;

  while ( ! __dead_clients.empty() ) {
    dcit = __dead_clients.begin();

    if ( __thread_collector ) {
      // ThreadCollector::remove also stops the threads!
      __thread_collector->remove(*dcit);
    } else {
      (*dcit)->cancel();
      (*dcit)->join();
    }
    if ( (__cit = find(__clients.begin(), __clients.end(), *dcit)) != __clients.end() ) {
      __clients.erase(__cit);
    }

    FuseServerClientThread *tc = *dcit;
    __dead_clients.erase(dcit);
    delete tc;
  }

  __clients.unlock();
  __dead_clients.unlock();  
}
