
/***************************************************************************
 *  sharpen.cpp - Implementation of the sharpen filter
 *
 *  Created: Thu Jun 16 16:13:15 2005
 *  Copyright  2005-2012  Tim Niemueller [www.niemueller.de]
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

#include <core/exception.h>
#include <fvfilters/sharpen.h>

#ifdef HAVE_IPP
#	include <ippi.h>
#elif defined(HAVE_OPENCV)
#	include <opencv2/opencv.hpp>
#else
#	error "Neither IPP nor OpenCV available"
#endif

namespace firevision {

/** @class FilterSharpen <fvfilters/sharpen.h>
 * Sharpen filter.
 * @author Tim Niemueller
 */

/** Constructor. */
FilterSharpen::FilterSharpen() : Filter("FilterSharpen")
{
}

void
FilterSharpen::apply()
{
#if defined(HAVE_IPP)
	IppiSize size;
	size.width  = src_roi[0]->width;
	size.height = src_roi[0]->height;

	IppStatus status;

	//                                   base + number of bytes to line y              + pixel bytes
	status = ippiFilterSharpen_8u_C1R(src[0] + (src_roi[0]->start.y * src_roi[0]->line_step)
	                                    + (src_roi[0]->start.x * src_roi[0]->pixel_step),
	                                  src_roi[0]->line_step,
	                                  dst + (dst_roi->start.y * dst_roi->line_step)
	                                    + (dst_roi->start.x * dst_roi->pixel_step),
	                                  dst_roi->line_step,
	                                  size);

	if (status != ippStsNoErr) {
		throw fawkes::Exception("Sharpen filter failed with %i\n", status);
	}
#elif defined(HAVE_OPENCV)
	if ((dst == NULL) || (dst == src[0])) {
		throw fawkes::Exception("OpenCV-based Sobel filter cannot be in-place");
	}

	cv::Mat srcm(src_roi[0]->height,
	             src_roi[0]->width,
	             CV_8UC1,
	             src[0] + (src_roi[0]->start.y * src_roi[0]->line_step)
	               + (src_roi[0]->start.x * src_roi[0]->pixel_step),
	             src_roi[0]->line_step);

	cv::Mat dstm(dst_roi->height,
	             dst_roi->width,
	             CV_8UC1,
	             dst + (dst_roi->start.y * dst_roi->line_step)
	               + (dst_roi->start.x * dst_roi->pixel_step),
	             dst_roi->line_step);

	cv::Mat kernel(3, 3, CV_32F);
	float * kernel_f = (float *)kernel.ptr();
	kernel_f[0]      = -0.125;
	kernel_f[1]      = -0.125;
	kernel_f[2]      = -0.125;
	kernel_f[3]      = -0.125;
	kernel_f[4]      = 2.0;
	kernel_f[5]      = -0.125;
	kernel_f[6]      = -0.125;
	kernel_f[7]      = -0.125;
	kernel_f[8]      = -0.125;

	cv::Point kanchor(1, 1);

	cv::filter2D(srcm, dstm, /* ddepth */ -1, kernel, kanchor);

#endif
}

} // end namespace firevision
