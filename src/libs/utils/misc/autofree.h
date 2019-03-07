
/***************************************************************************
 *  autofree.h - Automatic Freeer
 *
 *  Created: Thu Nov 26 13:15:50 2009
 *  Copyright  2005-2009  Tim Niemueller [www.niemueller.de]
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

#ifndef _UTILS_MISC_AUTOFREE_H_
#define _UTILS_MISC_AUTOFREE_H_

namespace fawkes {

class MemAutoFree
{
public:
	MemAutoFree(void *ptr);
	~MemAutoFree();

	void release();
	void reset(void *new_ptr);

	void *operator*() const;

private:
	void *ptr_;
};

} // end namespace fawkes

#endif
