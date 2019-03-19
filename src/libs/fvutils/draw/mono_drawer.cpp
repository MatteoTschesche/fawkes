
/***************************************************************************
 *  mono_drawer.cpp - Utility to draw in a buffer
 *
 *  Generated: Wed Feb 08 20:55:38 2006
 *  Copyright  2005-2007  Tim Niemueller [www.niemueller.de]
 *             2010  Bahram Maleki-Fard
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

#include <fvutils/color/yuv.h>
#include <fvutils/draw/mono_drawer.h>

#include <algorithm>
#include <cmath>
#include <unistd.h>

#define PUT_POINT(x, y)                                                               \
	{                                                                                   \
		if (overlap_)                                                                     \
			buffer_[y * width_ + x] = std::min(255, buffer_[y * width_ + x] + brightness_); \
		else                                                                              \
			buffer_[y * width_ + x] = brightness_;                                          \
	}

namespace firevision {

/** @class MonoDrawer <fvutils/draw/mono_drawer.h>
 * Draw to a monochrome image.
 * @author Tim Niemueller (Base)
 * @author Bahram Maleki-Fard (Modification)
 */

/** Constructor. */
MonoDrawer::MonoDrawer()
{
	buffer_     = NULL;
	brightness_ = 1;
	overlap_    = 1;
}

/** Destructor */
MonoDrawer::~MonoDrawer()
{
}

/** Set the buffer to draw to
 * @param buffer buffer to draw to, must be MONO8 formatted. E.g. Y-plane of YUV
 * @param width width of the buffer
 * @param height height of the buffer
 */
void
MonoDrawer::set_buffer(unsigned char *buffer, unsigned int width, unsigned int height)
{
	this->buffer_ = buffer;
	this->width_  = width;
	this->height_ = height;
}

/** Set drawing brightness.
 * @param b brightness; 0-255
 */
void
MonoDrawer::set_brightness(unsigned char b)
{
	brightness_ = b;
}

/** Enable/Disable transparency (overlapping pixels increase brightness).
 * @param o overlapping true/false
 */
void
MonoDrawer::set_overlap(bool o)
{
	overlap_ = o;
}

/** Draw circle.
 * Draws a circle at the given center point and with the given radius.
 * @param center_x x coordinate of circle center
 * @param center_y y coordinate of circle center
 * @param radius radius of circle
 */
void
MonoDrawer::draw_circle(int center_x, int center_y, unsigned int radius)
{
	if (buffer_ == NULL)
		return;

	unsigned int x = 0, y = radius, r2 = radius * radius;

	unsigned int x_tmp, y_tmp;

	while (x <= y) {
		x_tmp = center_x + x;
		y_tmp = center_y + y;
		if ((x_tmp < width_) && (y_tmp < height_))
			PUT_POINT(x_tmp, y_tmp);

		x_tmp = center_x - x;
		y_tmp = center_y + y;
		if ((x_tmp < width_) && (y_tmp < height_))
			PUT_POINT(x_tmp, y_tmp);

		x_tmp = center_x + y;
		y_tmp = center_y + x;
		if ((x_tmp < width_) && (y_tmp < height_))
			PUT_POINT(x_tmp, y_tmp);

		x_tmp = center_x - y;
		y_tmp = center_y + x;
		if ((x_tmp < width_) && (y_tmp < height_))
			PUT_POINT(x_tmp, y_tmp);

		x_tmp = center_x + x;
		y_tmp = center_y - y;
		if ((x_tmp < width_) && (y_tmp < height_))
			PUT_POINT(x_tmp, y_tmp);

		x_tmp = center_x - x;
		y_tmp = center_y - y;
		if ((x_tmp < width_) && (y_tmp < height_))
			PUT_POINT(x_tmp, y_tmp);

		x_tmp = center_x + y;
		y_tmp = center_y - x;
		if ((x_tmp < width_) && (y_tmp < height_))
			PUT_POINT(x_tmp, y_tmp);

		x_tmp = center_x - y;
		y_tmp = center_y - x;
		if ((x_tmp < width_) && (y_tmp < height_))
			PUT_POINT(x_tmp, y_tmp);

		++x;
		y = (int)(sqrt((float)(r2 - x * x)) + 0.5);
	}
}

/** Draw rectangle.
 * @param x x coordinate of rectangle's upper left corner
 * @param y y coordinate of rectangle's upper left corner
 * @param w width of rectangle from x to the right
 * @param h height of rectangle from y to the bottom
 */
void
MonoDrawer::draw_rectangle(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
	// horizontal line at top
	for (unsigned int i = x; i < x + w; ++i) {
		if (i < width_) {
			PUT_POINT(i, y);
		} else {
			break;
		}
	}

	// left and right
	for (unsigned int i = y; i < y + h; ++i) {
		// left
		PUT_POINT(x, i);

		if ((x + w) < width_) {
			// right
			PUT_POINT(x + w, i);
		}
	}

	// horizontal line at bottom
	for (unsigned int i = x; i < x + w; ++i) {
		if (i < width_) {
			PUT_POINT(i, y + h);
		} else {
			break;
		}
	}
}

/** Draw inverted rectangle.
 * This draws a rectangle but instead of using the draw color it is drawn
 * in the inverted color of the pixel where it is drawn.
 * @param x x coordinate of rectangle's upper left corner
 * @param y y coordinate of rectangle's upper left corner
 * @param w width of rectangle from x to the right
 * @param h height of rectangle from y to the bottom
 */
void
MonoDrawer::draw_rectangle_inverted(unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
	unsigned int ind = 0;

	// horizontal line at top
	for (unsigned int i = x; i < x + w; ++i) {
		if (i < width_) {
			ind          = y * width_ + i;
			buffer_[ind] = 255 - buffer_[ind];
		} else {
			break;
		}
	}

	// left and right
	for (unsigned int i = y; i < y + h; ++i) {
		// left
		ind          = i * width_ + x;
		buffer_[ind] = 255 - buffer_[ind];

		if ((x + w) < width_) {
			// right
			ind += w;
			buffer_[ind] = 255 - buffer_[ind];
		}
	}

	// horizontal line at bottom
	for (unsigned int i = x; i < x + w; ++i) {
		if (i < width_) {
			buffer_[ind] = 255 - buffer_[ind];
		} else {
			break;
		}
	}
}

/** Draw point.
 * @param x x coordinate of point
 * @param y y coordinate of point
 */
void
MonoDrawer::draw_point(unsigned int x, unsigned int y)
{
	if (x > width_)
		return;
	if (y > height_)
		return;

	PUT_POINT(x, y);
}

/** Draw line.
 * Standard Bresenham in all directions. For in-depth information
 * have a look at http://de.wikipedia.org/wiki/Bresenham-Algorithmus
 * @param x_start x coordinate of start point
 * @param y_start y coordinate of start point
 * @param x_end x coordinate of end point
 * @param y_end y coordinate of end point
 */
void
MonoDrawer::draw_line(unsigned int x_start,
                      unsigned int y_start,
                      unsigned int x_end,
                      unsigned int y_end)
{
	/* heavily inspired by an article on German Wikipedia about
   * Bresenham's algorithm, confer
   * http://de.wikipedia.org/wiki/Bresenham-Algorithmus
   */

	int  x, y, dist, xerr, yerr, dx, dy, incx, incy;
	bool was_inside_image = false;

	// calculate distance in both directions
	dx = x_end - x_start;
	dy = y_end - y_start;

	// Calculate sign of the increment
	if (dx < 0) {
		incx = -1;
		dx   = -dx;
	} else {
		incx = dx ? 1 : 0;
	}

	if (dy < 0) {
		incy = -1;
		dy   = -dy;
	} else {
		incy = dy ? 1 : 0;
	}

	// check which distance is larger
	dist = (dx > dy) ? dx : dy;

	// Initialize for loops
	x    = x_start;
	y    = y_start;
	xerr = dx;
	yerr = dy;

	/* Calculate and draw pixels */
	for (int t = 0; t < dist; ++t) {
		if (((unsigned int)x < width_) && ((unsigned int)y < height_)) {
			if ((x >= 0) && (y >= 0)) {
				was_inside_image = true;
				PUT_POINT(x, y);
			}
		} else {
			if (was_inside_image) {
				break;
			}
		}

		xerr += dx;
		yerr += dy;

		if (xerr > dist) {
			xerr -= dist;
			x += incx;
		}

		if (yerr > dist) {
			yerr -= dist;
			y += incy;
		}
	}

	if ((x_end < width_) && (y_end < height_)) {
		PUT_POINT(x_end, y_end);
	}
}

/** Draws a cross.
 * @param x_center Center of the cross
 * @param y_center Center of the cross
 * @param width of the bars
 */
void
MonoDrawer::draw_cross(unsigned int x_center, unsigned int y_center, unsigned int width)
{
	x_center = std::min(x_center, width_);
	y_center = std::min(y_center, height_);

	int          r = width / 2;
	unsigned int a = std::max(0, (int)x_center - r);
	unsigned int b = std::min(x_center + r, width_);
	draw_line(a, y_center, b, y_center);

	a = std::max(0, (int)y_center - r);
	b = std::min(y_center + r, height_);
	draw_line(x_center, a, x_center, b);
}
} // end namespace firevision
