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
#include <cassert>
#include <climits>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <boost/container/small_vector.hpp>

#include "mcell_structs.h"
#include "debug_config.h"

// warning: do not use floating point types from directly,
// we need to be able to control the precision
#include "../libs/glm/glm.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "../libs/glm/gtx/component_wise.hpp"

// this file must not depend on any other from mcell4 otherwise there
// might be some nasty cyclic include dependencies

namespace mcell {

// ---------------------------------- optimization macros ----------------------------------
#if defined(likely) or defined(unlikely)
#error "Macros 'likely' or 'unlikely' are already defined"
#endif

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

// ---------------------------------- float types ----------------------------------

typedef double float_t; // will be changed to float
#define FLOAT_T_BYTES 8

#if FLOAT_T_BYTES == 8
const float_t EPS = 1e-12; // same as EPS_C
const float_t SQRT_EPS = 1e-6;
#else
#error "TODO: float32"
#endif

const float_t SCHEDULER_COMPARISON_EPS = 1e-10;

typedef glm::dvec3 glm_vec3_t;
typedef glm::dvec2 glm_vec2_t;
typedef glm::ivec3 ivec3_t;
typedef glm::uvec3 uvec3_t;
typedef glm::bvec3 bvec3_t;
typedef glm::dmat4x4 mat4x4;

struct vec3_t: public glm_vec3_t{
  vec3_t() : glm_vec3_t(0) {}
  vec3_t(const glm_vec3_t& a) { x = a.x; y = a.y; z = a.z; }
  vec3_t(const vec3_t& a) : glm_vec3_t(a.x, a.y, a.z) { }
  vec3_t(const vector3& a) { x = a.x; y = a.y; z = a.z; }
  vec3_t(const float_t x_, const float_t y_, const float_t z_) { x = x_; y = y_; z = z_; }
  vec3_t(const float_t xyz) { x = xyz; y = xyz; z = xyz; }

  std::string to_string() const;
  void dump(const std::string extra_comment, const std::string ind) const;

  // for exd_vector3
  float_t& m() { return x; }
  float_t& u() { return y; }
  float_t& v() { return z; }

  const float_t& m() const { return x; }
  const float_t& u() const { return y; }
  const float_t& v() const { return z; }
};

// usually are .u and .v used to access cotained values
struct vec2_t: public glm_vec2_t{
  vec2_t() : glm_vec2_t(0) {}
  vec2_t(const glm_vec2_t& a) { x = a.x; y = a.y; }
  vec2_t(const vec2_t& a) : glm_vec2_t(a.x, a.y) { }
  vec2_t(const vector2& a) { x = a.u; y = a.v; }
  vec2_t(const float_t x_, const float_t y_) { x = x_; y = y_; }
  vec2_t(const float_t xy) { x = xy; y = xy; }

  //TODO: std::string to_string() const;
  //TODO: void dump(const std::string extra_comment, const std::string ind) const;

  float_t& u() { return x; }
  float_t& v() { return y; }

  const float_t& u() const { return x; }
  const float_t& v() const { return y; }
};

std::ostream & operator<<(std::ostream &out, const vec3_t &a);


// ---------------------------------- configurable constants----------------------------------

const uint32_t DEFRAGMENTATION_PERIODICITY = 500;
const float_t PARTITION_EDGE_LENGTH_DEFAULT = 10 * 100 /*100 = 1/length unit*/; // large for now because we have just one partition
const float_t SUBPARTITIONS_PER_PARTITION_DIMENSION_DEFAULT = 1;


// ---------------------------------- fixed costants and specific typedefs -------------------
const float_t POS_INVALID = NAN;

const float_t TIME_INVALID = NAN;
const float_t TIME_FOREVER = FLT_MAX; // this max is sufficient for both float and double
const float_t TIME_SIMULATION_START = 0;

// unique species id
typedef uint32_t species_id_t;
const uint32_t SPECIES_ID_INVALID = UINT32_MAX;

// molecule id is a unique identifier of a molecule,
// no 2 molecules may have the same ID in the course of a simulation (at least for now)
typedef uint32_t molecule_id_t;
const uint32_t MOLECULE_ID_INVALID = UINT32_MAX;

// molecule index is index into partition's molecules array, indices and ids are
// identical until the first defragmentation that shuffles molecules in the molecules array
const uint32_t MOLECULE_INDEX_INVALID = UINT32_MAX;

// for now, this is the partition that contains point 0,0,0 in its center
typedef uint32_t partition_index_t;
const partition_index_t PARTITION_INDEX_INITIAL = 0;
const partition_index_t PARTITION_INDEX_INVALID = UINT32_MAX;

typedef uint32_t subpart_index_t;
const subpart_index_t SUBPART_INDEX_INVALID = UINT32_MAX;

// time step is used in parition to make sets of molecules that can be diffused with
// different periodicity
const uint32_t TIME_STEP_INDEX_INVALID = UINT32_MAX;

const char* const NAME_INVALID = "invalid_name";

const uint64_t BUCKET_INDEX_INVALID = UINT64_MAX;

const uint32_t VERTICES_IN_TRIANGLE = 3;

typedef uint32_t vertex_index_t; // index in partition's vertices
typedef uint32_t wall_index_t; // index in partition's walls
const wall_index_t WALL_INDEX_INVALID = UINT32_MAX;

typedef uint32_t wall_id_t; // world-unique wall id
//typedef uint32_t wall_class_index_t; // index in world's wall classes
typedef uint32_t geometry_object_index_t;
typedef uint32_t geometry_object_id_t; // world-unique unique geometry object id

/*
struct partition_vertex_index_pair_t {
  partition_index_t first;
  vertex_index_t second;
};
*/
typedef std::pair<partition_index_t, vertex_index_t> partition_vertex_index_pair_t;

typedef boost::container::small_vector<subpart_index_t, 8>  subpart_indices_vector_t;

// ---------------------------------- auxiliary functions ----------------------------------

static inline float_t floor_to_multiple(const float_t val, float_t multiple) {
  return (float_t)((int)(val / multiple)) * multiple;
}

static inline vec3_t floor_to_multiple(const vec3_t& val, float_t multiple) {
  return (vec3_t)((glm::ivec3)(val / multiple)) * multiple;
}

static inline bool cmp_eq(const float_t a, const float_t b, const float_t eps) {
  return fabs(a - b) < eps;
}

static inline bool cmp_lt(const float_t a, const float_t b, const float_t eps) {
  return a < b && !cmp_eq(a, b, eps);
}

static inline uint32_t powu(const uint32_t a, const uint32_t n) {
  uint32_t res = a;
  for (uint32_t i = 1; i < n; i++) {
    res *= a;
  }
  return res;
}

static inline float_t max3d(const vec3_t& v) {
  return glm::compMax((glm_vec3_t)v);
}

/* abs_max_2vec picks out the largest (absolute) value found among two vectors
 * (useful for properly handling floating-point rounding error). */
static inline float_t abs_max_2vec(const vec3_t& v1, const vec3_t& v2) {
  glm_vec3_t v1abs = glm::abs((glm_vec3_t)v1);
  glm_vec3_t v2abs = glm::abs((glm_vec3_t)v2);
  vec3_t vmax = glm::max(v1abs, v2abs);
  return mcell::max3d(vmax);
}

static inline float_t dot2(const vec2_t& v1, const vec2_t& v2) {
  return glm::dot((glm_vec2_t)v1, (glm_vec2_t)v2);
}

static inline float_t determinant2(const vec2_t& v1, const vec2_t& v2) {
  return v1.x * v2.y - v1.y * v2.x;
}

static inline float_t len2_squared(const vec2_t& v) {
  return v.x * v.x + v.y * v.y;
}

static inline float_t dot(const vec3_t& v1, const vec3_t& v2) {
  return glm::dot((glm_vec3_t)v1, (glm_vec3_t)v2);
}

// returns true when whether two values are measurably different
// FIXME: has the same signature as mcell3 version
inline bool distinguishable(float_t a, float_t b, float_t eps) {
  float_t c = fabs(a - b);
  a = fabs(a);
  if (a < 1) {
    a = 1;
  }
  b = fabs(b);

  if (b < a) {
    eps *= a;
  } else {
    eps *= b;
  }
  return (c > eps);
}


static inline bool distinguishable_vec3(const vec3_t& a, const vec3_t& b, float_t eps) {
  float_t c, cc, d;

  /* Find largest coordinate */
  c = fabs(a.x);

  d = fabs(a.y);
  if (d > c)
    c = d;

  d = fabs(a.z);
  if (d > c)
    c = d;

  d = fabs(b.x);
  if (d > c)
    c = d;

  d = fabs(b.y);
  if (d > c)
    c = d;

  d = fabs(b.z);
  if (d > c)
    c = d;

  /* Find largest difference */
  cc = fabs(a.x - b.x);

  d = fabs(a.y - b.y);
  if (d > cc)
    cc = d;

  d = fabs(a.z - b.z);
  if (d > cc)
    cc = d;

  /* Make sure fractional difference is at least eps and absolute difference is
   * at least (eps*eps) */
  if (c < eps)
    c = eps;
  return (c * eps < cc);
}


static inline void debug_guard_zero_div(float_t& val) {
#ifndef NDEBUG
  // if we divide by such a small number, result is practically the same as
  // if we would return inf during division
  if (val == 0) {
    val = FLT_MIN;
  }
#endif
}

static inline void debug_guard_zero_div(vec3_t& val) {
#ifndef NDEBUG
  if (val.x == 0) {
    val.x = FLT_MIN;
  }
  if (val.y == 0) {
    val.y = FLT_MIN;
  }
  if (val.z == 0) {
    val.z = FLT_MIN;
  }
#endif
}

// ---------------------------------- world_constants_t ----------------------------------
// TODO: maybe move to a separate header
// TODO: move reactions to be owned by world constants

#if 0
//not used yet
/**.
 * Class of a wall.
 * Although usually an object has the same properties, some of it regions might act differently.
 * This class serves to store this constant type of information for all items in the world.
 */
class wall_class_t {
public:
  // absobtive, ...
  // what else could I need here?
  uint32_t flags;
};
#endif


class reaction_t;
typedef std::unordered_map<species_id_t, reaction_t*> species_reaction_map_t;
typedef species_reaction_map_t unimolecular_reactions_map_t;
typedef std::unordered_map< species_id_t, species_reaction_map_t > bimolecular_reactions_map_t;

/*
 * Constant data set in initialization useful for all classes, single object is owned by world
 */
struct world_constants_t {
  // configuration
  float_t time_unit;
  float_t length_unit;
  float_t rx_radius_3d;
  float_t partition_edge_length;
  uint32_t subpartitions_per_partition_dimension;
  uint32_t subpartitions_per_partition_dimension_squared;
  float_t subpartition_edge_length; // == partition_edge_length / subpartitions_per_partition_dimension
  float_t subpartition_edge_length_rcp; // == 1/subpartition_edge_length

  // other options
  bool use_expanded_list;


  const unimolecular_reactions_map_t* unimolecular_reactions_map; // owned by world
  const bimolecular_reactions_map_t* bimolecular_reactions_map; // owned by world

private:
  void init_subpartition_edge_length() {
    if (partition_edge_length != 0) {
      subpartition_edge_length = partition_edge_length / (float_t)subpartitions_per_partition_dimension;
      subpartition_edge_length_rcp = 1.0/subpartition_edge_length;
    }
    subpartitions_per_partition_dimension_squared = powu(subpartitions_per_partition_dimension, 2);
  }

public:
  // called from world::init_simulation()
  void init(
      unimolecular_reactions_map_t* unimolecular_reactions_map_,
      bimolecular_reactions_map_t* bimolecular_reactions_map_
      ) {
    unimolecular_reactions_map = unimolecular_reactions_map_;
    bimolecular_reactions_map = bimolecular_reactions_map_;
    init_subpartition_edge_length();
  }

  void dump();

  // TODO: maybe add: bool fully_initialized;
};

/*
 * Constant data set in initialization useful for all classes, single object is owned by world
 */
struct simulation_stats_t {
  simulation_stats_t()
    : ray_voxel_tests(0), ray_polygon_tests(0), ray_polygon_colls(0) {
  }
  void inc_ray_voxel_tests() {
    ray_voxel_tests++;
  }
  void inc_ray_polygon_tests() {
    ray_polygon_tests++;
  }
  void inc_ray_polygon_colls() {
    ray_polygon_colls++;
  }

  void dump();
private:
  uint64_t ray_voxel_tests;
  uint64_t ray_polygon_tests;
  uint64_t ray_polygon_colls;
};

} // namespace mcell

#endif // SRC4_DEFINES_H_
