
/***************************************************************************
 *  deadspots.cpp - Laser data dead spots filter
 *
 *  Created: Wed Jun 24 22:42:51 2009
 *  Copyright  2006-2011  Tim Niemueller [www.niemueller.de]
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

#include "deadspots.h"

#include <config/config.h>
#include <core/exception.h>
#include <core/macros.h>
#include <logging/logger.h>
#include <sys/types.h>
#include <utils/math/angle.h>
#include <utils/time/time.h>

#include <cstdlib>
#include <cstring>
#include <regex.h>

using namespace fawkes;

/** @class LaserDeadSpotsDataFilter "filters/deadspots.h"
 * Erase dead spots (i.e. mounting rods in the laser range) from laser data.
 * This filter reads a number of values stored in /hardware/laser/deadspots, where
 * each dead spot must contain two entries, a start and an end in degrees. Each
 * entry is stored as submembers of the given tree, for example as
 * /hardware/laser/deadspots/0/start and /hardware/laser/deadspots/0/end.
 * @author Tim Niemueller
 */

/** Constructor.
 * @param filter_name name of this filter instance
 * @param config configuration instance
 * @param logger logger for informational output
 * @param prefix configuration prefix where to log for config information
 * @param in_data_size number of entries input value arrays
 * @param in vector of input arrays
 */
LaserDeadSpotsDataFilter::LaserDeadSpotsDataFilter(const std::string &    filter_name,
                                                   fawkes::Configuration *config,
                                                   fawkes::Logger *       logger,
                                                   const std::string &    prefix,
                                                   unsigned int           in_data_size,
                                                   std::vector<LaserDataFilter::Buffer *> &in)
: LaserDataFilter(filter_name, in_data_size, in, in.size())
{
	logger_ = logger;

	regex_t pathre;
	int     error = 0;
	if ((error = regcomp(&pathre, (prefix + "\\([^/]\\+\\)/\\(start\\|end\\)").c_str(), 0)) != 0) {
		size_t errsize = regerror(error, &pathre, NULL, 0);
		char   tmp[errsize];
		regerror(error, &pathre, tmp, errsize);
		regfree(&pathre);
		throw Exception("Failed to compile regular expression: %s", tmp);
	}

	regmatch_t matches[2];

	std::list<std::string> entries;

	Configuration::ValueIterator *vit = config->search(prefix.c_str());
	while (vit->next()) {
		const char *path = vit->path();
		if (regexec(&pathre, path, 2, matches, 0) == 0) {
			size_t match1_length = matches[1].rm_eo - matches[1].rm_so;

			char entry[match1_length + 1];
			entry[match1_length] = 0;
			strncpy(entry, &(path[matches[1].rm_so]), match1_length);
			entries.push_back(entry);
		}
	}
	delete vit;
	entries.sort();
	entries.unique();

	dead_spots_size_ = entries.size() * 2;
	dead_spots_      = new unsigned int[dead_spots_size_];

	for (std::list<std::string>::iterator i = entries.begin(); i != entries.end(); ++i) {
		std::string path  = prefix + *i + "/";
		float       start = config->get_float((path + "start").c_str());
		float       end   = config->get_float((path + "end").c_str());

		logger_->log_debug(
		  "LaserDeadSpotsDataFilter", "Adding dead range [%3.3f, %3.3f] (%s)", start, end, i->c_str());
		cfg_dead_spots_.push_back(std::make_pair(start, end));
	}

	num_spots_ = cfg_dead_spots_.size();

	if (num_spots_ == 0) {
		throw Exception(
		  "Dead spots filter enabled but no calibration data exists. Run fflaser_deadspots.");
	}

	calc_spots();
}

/** Constructor.
 * @param other instance to copy from
 */
LaserDeadSpotsDataFilter::LaserDeadSpotsDataFilter(const LaserDeadSpotsDataFilter &other)
: LaserDataFilter(other.filter_name, other.in_data_size, other.in, other.in.size()),
  logger_(other.logger_),
  num_spots_(other.num_spots_),
  dead_spots_size_(other.dead_spots_size_),
  cfg_dead_spots_(other.cfg_dead_spots_)
{
	dead_spots_ = new unsigned int[dead_spots_size_];
	for (unsigned int i = 0; i < dead_spots_size_; ++i) {
		dead_spots_[i] = other.dead_spots_[i];
	}
}

LaserDeadSpotsDataFilter::~LaserDeadSpotsDataFilter()
{
	delete[] dead_spots_;
}

/** Assignment operator
 * @param other instance to copy from
 * @return reference to this instance
 */
LaserDeadSpotsDataFilter &
LaserDeadSpotsDataFilter::operator=(const LaserDeadSpotsDataFilter &other)
{
	if (&other == this)
		return *this;

	delete[] dead_spots_;

	filter_name  = other.filter_name;
	in_data_size = other.in_data_size;
	in           = other.in;
	logger_      = other.logger_;

	cfg_dead_spots_  = other.cfg_dead_spots_;
	num_spots_       = other.num_spots_;
	dead_spots_size_ = other.dead_spots_size_;
	dead_spots_      = new unsigned int[dead_spots_size_];
	for (unsigned int i = 0; i < dead_spots_size_; ++i) {
		dead_spots_[i] = other.dead_spots_[i];
	}

	return *this;
}

void
LaserDeadSpotsDataFilter::set_out_vector(std::vector<LaserDataFilter::Buffer *> &out)
{
	LaserDataFilter::set_out_vector(out);
	calc_spots();
}

void
LaserDeadSpotsDataFilter::calc_spots()
{
	if (in_data_size != out_data_size) {
		throw Exception("Dead spots filter requires equal input and output data size");
	}

	// need to calculate new beam ranges and allocate different memory segment
	float angle_factor = 360.0 / in_data_size;
	for (unsigned int i = 0; i < num_spots_; ++i) {
		dead_spots_[i * 2] =
		  std::min(in_data_size - 1, (unsigned int)ceilf(cfg_dead_spots_[i].first / angle_factor));
		dead_spots_[i * 2 + 1] =
		  std::min(in_data_size - 1, (unsigned int)ceilf(cfg_dead_spots_[i].second / angle_factor));
	}
}

void
LaserDeadSpotsDataFilter::filter()
{
	const unsigned int vecsize = std::min(in.size(), out.size());
	for (unsigned int a = 0; a < vecsize; ++a) {
		out[a]->frame = in[a]->frame;
		out[a]->timestamp->set_time(in[a]->timestamp);
		float *inbuf  = in[a]->values;
		float *outbuf = out[a]->values;

		unsigned int start = 0;
		for (unsigned int i = 0; i < num_spots_; ++i) {
			const unsigned int spot_start = dead_spots_[i * 2];
			const unsigned int spot_end   = dead_spots_[i * 2 + 1];
			for (unsigned int j = start; j < spot_start; ++j) {
				outbuf[j] = inbuf[j];
			}
			for (unsigned int j = spot_start; j <= spot_end; ++j) {
				outbuf[j] = 0.0;
			}
			start = spot_end + 1;
		}
		for (unsigned int j = start; j < in_data_size; ++j) {
			outbuf[j] = inbuf[j];
		}
	}
}
