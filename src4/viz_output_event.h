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

#ifndef SRC4_VIZ_OUTPUT_EVENT_H_
#define SRC4_VIZ_OUTPUT_EVENT_H_

#include "base_event.h"

namespace mcell {

/**
 * Dumps world state either in a textual or cellblender format.
 */
class viz_output_event_t: public base_event_t {
public:
  viz_output_event_t(world_t* world_)
    : base_event_t(EVENT_TYPE_INDEX_VIZ_OUTPUT),
      viz_mode(NO_VIZ_MODE), file_prefix_name(nullptr),
      world(world_) {
  }
  virtual ~viz_output_event_t() {}

  virtual void step();
  virtual void dump(const std::string indent);

  viz_mode_t viz_mode;
  const char* file_prefix_name; // in const pool

  world_t* world;

private:
  FILE* create_and_open_output_file_name();
  void output_ascii_molecules();
  void output_cellblender_molecules();
};

} // namespace mcell

#endif // SRC4_VIZ_OUTPUT_EVENT_H_
