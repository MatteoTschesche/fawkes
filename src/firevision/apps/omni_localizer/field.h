/*
    Copyright (c) 2008 Volker Krause <volker.krause@rwth-aachen.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef FVOMNILOCALIZER_FIELD_H
#define FVOMNILOCALIZER_FIELD_H

#include <fvutils/base/types.h>

#include <vector>
#include <fstream>

class Configuration;

/** Represents an arc. */
struct arc_t
{
  /** Arc center. */
  f_point_t m;
  /** Arc radius. */
  float r;
  /** Left angle. */
  float left;
  /** Right angle. */
  float right;
};

class Field
{
  public:
    Field( Configuration *config );
    ~Field();

    void load( const char *filename );
    void save( const char *filename );

    float fieldWidth() const;
    float fieldHeight() const;

    float totalWidth() const;
    float totalHeight() const;

    std::vector<float> findIntersections( const field_pos_t &position, float phi );
    float weightForDistance( float lineDistance, float sensorDistance ) const;

    void setDebugBuffer( unsigned char *buffer, unsigned int width = 0, unsigned int height = 0 );
    void drawField();
    void dumpSensorProbabilities( const field_pos_t &position, const char* filename );

  private:
    std::vector< std::pair<f_point_t, f_point_t> > mLines;
    std::vector< arc_t > mArcs;

    float mFieldWidth, mFieldHeight, mTotalWidth, mTotalHeight;

    unsigned char *mDebugBuffer;
    unsigned int mWidth, mHeight;

    float mUpperRange, mLowerRange;
};

#endif
