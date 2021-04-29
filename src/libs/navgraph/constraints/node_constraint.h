/***************************************************************************
 *  node_constraint.h - base class for nod constraints
 *
 *  Created: Sun Mar 02 10:47:35 2014
 *  Copyright  2014  Sebastian Reuter
 *             2014  Tim Niemueller
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

#ifndef _NAVGRAPH_CONSTRAINTS_NODE_CONSTRAINT_H_
#define _NAVGRAPH_CONSTRAINTS_NODE_CONSTRAINT_H_

#include <navgraph/navgraph_node.h>

#include <string>
#include <vector>

namespace fawkes {

class Logger;

class NavGraphNodeConstraint
{
public:
	NavGraphNodeConstraint(const std::string &name);
	NavGraphNodeConstraint(const char *name);
	virtual ~NavGraphNodeConstraint();

	std::string name();

	virtual bool compute(void) noexcept;
	virtual bool blocks(const fawkes::NavGraphNode &node) noexcept = 0;

	bool operator==(const std::string &name) const;

protected:
	std::string name_;
};

} // end namespace fawkes

#endif
