/***************************************************************************
 *  skiller_plugin.h - Simulate the execution of a skill
 *
 *  Created: Mon 06 May 2019 09:01:39 CEST 09:01
 *  Copyright  2019  Till Hofmann <hofmann@kbsg.rwth-aachen.de>
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

#ifndef _PLUGINS_SKILLER_SIMULATOR_EXEC_PLUGIN_H_
#define _PLUGINS_SKILLER_SIMULATOR_EXEC_PLUGIN_H_

#include <core/plugin.h>

class SkillerSimulatorPlugin : public fawkes::Plugin
{
public:
	explicit SkillerSimulatorPlugin(fawkes::Configuration *config);
};

#endif /* !_PLUGINS_SKILLER_SIMULATOR_EXEC_PLUGIN_H_ */
