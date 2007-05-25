
/***************************************************************************
 *  finalize_nettler_thread.h - Fawkes Example Plugin Finalize Nettler Thread
 *
 *  Created: Thu May 24 00:32:22 2007
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

#ifndef __PLUGINS_EXAMPLE_FINALIZE_NETTLER_THREAD_H_
#define __PLUGINS_EXAMPLE_FINALIZE_NETTLER_THREAD_H_

#include <core/threading/thread.h>
#include <aspect/blocked_timing.h>
#include <aspect/logging.h>

class ExampleFinalizeNettlerThread : public Thread, public LoggingAspect
{

 public:
  ExampleFinalizeNettlerThread(const char *name);
  virtual ~ExampleFinalizeNettlerThread();

  virtual void init();
  virtual void loop();

  virtual bool prepare_finalize_user();
  virtual void finalize();

 private:
  bool nagged;
};


#endif
