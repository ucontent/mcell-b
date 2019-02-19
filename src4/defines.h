/******************************************************************************
 *
 * Copyright (C) 2019 by
 * The Salk Institute for Biological Studies and
 * Pittsburgh Supercomputing Center, Carnegie Mellon University
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 *
******************************************************************************/

#ifndef SRC4_DEFINES_H_
#define SRC4_DEFINES_H_

#include <stdint.h>

#include <vector>
#include <string>
#include <bitset>
#include <cassert>
#include <climits>
#include <cmath>
#include <iostream>

#include "mcell_structs.h"

// warning: do not use directly, we need to be able to control the precision
#include "../libs/glm/glm.hpp"

namespace mcell {

typedef double float_t; // soon to be changed to float
const float_t TIME_INVALID = NAN;
const float_t TIME_SIMULATION_START = 0;

struct vec3_t {
	float_t x;
	float_t y;
	float_t z;

	vec3_t() : x(0), y(0), z(0) {}
	vec3_t(const vec3_t& a) { x = a.x; y = a.y; z = a.z; }
	vec3_t(const vector3& a) { x = a.x; y = a.y; z = a.z; }
	vec3_t(const float_t x_, const float_t y_, const float_t z_) { x = x_; y = y_; z = z_; }

	// exact match
	bool operator == (const vec3_t& a) { return x == a.x && y == a.y && z == a.z; }
};

std::ostream & operator<<(std::ostream &out, const vec3_t &a);


const int MAX_MOLECULES_PER_PARTITION = 32*32*32 /*32k*/; //temporary, must work dynamically

//typedef std::bitset<MAX_MOLECULES_PER_PARTITION> subpartition_mask_t;


typedef uint16_t species_id_t;
typedef uint32_t molecule_index_t;

const int SPECIES_ID_INVALID = USHRT_MAX;


typedef glm::dmat4x4 mat4x4;

} /* namespace mcell */

#endif // SRC4_DEFINES_H_
