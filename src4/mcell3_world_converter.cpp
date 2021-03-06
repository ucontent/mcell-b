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

#include <stdarg.h>
#include <stdlib.h>
#include <set>

extern "C" {
#include "logging.h"
}
#include "mcell_structs.h"
#include "mcell3_world_converter.h"

#include "world.h"
#include "release_event.h"
#include "diffuse_react_event.h"
#include "viz_output_event.h"

using namespace std;

// checking major converstion blocks
#define CHECK(a) do { if(!(a)) return false; } while (0)

// checking assumptions
#define CHECK_PROPERTY(cond) do { if(!(cond)) { mcell_log_conv_error("Expected '%s' is false. (%s - %s:%d)\n", #cond, __FUNCTION__, __FILE__, __LINE__); return false; } } while (0)

// asserts - things that can never occur and will 'never' be supported


// holds global class
mcell::mcell3_world_converter g_converter;


bool mcell4_convert_mcell3_volume(volume* s) {
  return g_converter.convert(s);
}


bool mcell4_run_simulation() {
  return g_converter.world->run_simulation();
}


void mcell4_delete_world() {
  return g_converter.reset();
}


void mcell_log_conv_warning(char const *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  string fmt_w_warning = string ("Conversion warning: ") + fmt;
  mcell_logv_raw(fmt_w_warning.c_str(), args);
  va_end(args);
}


void mcell_log_conv_error(char const *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  string fmt_w_warning = string ("Conversion error: ") + fmt;
  mcell_logv_raw(fmt_w_warning.c_str(), args);
  va_end(args);
}


namespace mcell {

static const char* get_sym_name(const sym_entry *s) {
  assert(s != nullptr);
  assert(s->name != nullptr);
  return s->name;
}



static mat4x4 t_matrix_to_mat4x4(const double src[4][4]) {
  mat4x4 res;

  for (int x = 0; x < 4; x++) {
    for (int y = 0; y < 4; y++) {
      res[x][y] = src[x][y];
    }
  }

  return res;
}


void mcell3_world_converter::reset() {
  delete world;
  world = nullptr;
  mcell3_species_id_map.clear();
}


bool mcell3_world_converter::convert(volume* s) {

  world = new world_t();

  CHECK(convert_simulation_setup(s));

  CHECK(convert_species_and_create_diffusion_events(s));
  CHECK(convert_reactions(s));

  world->init_world_constants();

  CHECK(convert_release_events(s));
  CHECK(convert_viz_output_events(s));

  // at this point, we need to create the first (and for now the only) partition
  // create initial partition with center at 0,0,0 - we woud like to have the partitions all the same,
  // not depend on some random initialization
  uint32_t index = world->add_partition(vec3_t(0, 0, 0));
  assert(index == PARTITION_INDEX_INITIAL);

  // convert geometry already puts geometry objects into partitions
  CHECK(convert_geometry_objects(s));

  return true;
}


bool mcell3_world_converter::convert_simulation_setup(volume* s) {
  // TODO: many items are not checked
  world->iterations = s->iterations;
  world->world_constants.time_unit = s->time_unit;
  world->world_constants.length_unit = s->length_unit;
  world->world_constants.rx_radius_3d = s->rx_radius_3d;
  world->seed_seq = s->seed_seq;
  world->rng = *s->rng;

  // there seems to be just one partition in MCell but we interpret it as mcell4 partition size
  if (s->partitions_initialized) {
    CHECK_PROPERTY(s->partition_llf[0] == s->partition_llf[1]);
    CHECK_PROPERTY(s->partition_llf[1] == s->partition_llf[2]);
    CHECK_PROPERTY(s->partition_urb[0] == s->partition_urb[1]);
    CHECK_PROPERTY(s->partition_urb[1] == s->partition_urb[2]);
    assert(s->partition_urb[0] > s->partition_llf[0]);
    world->world_constants.partition_edge_length = (s->partition_urb[0] - s->partition_llf[0]) / s->length_unit;
  }
  else {
    world->world_constants.partition_edge_length = PARTITION_EDGE_LENGTH_DEFAULT;
  }
  CHECK_PROPERTY(s->nx_parts == s->ny_parts);
  CHECK_PROPERTY(s->ny_parts == s->nz_parts);

  world->world_constants.use_expanded_list = s->use_expanded_list;

  // this number counts the number of boundaries, not subvolumes, also, there are always 2 extra subvolumes on the sides in mcell3
  world->world_constants.subpartitions_per_partition_dimension = s->nx_parts - 3;

  return true;
}


bool check_meta_object(object* o, string expected_name) {
  assert(o != nullptr);
  CHECK_PROPERTY(o->next == nullptr);
  CHECK_PROPERTY(get_sym_name(o->sym) == expected_name);
  // root->last_name - not checked, contains some nonsense anyway
  CHECK_PROPERTY(o->object_type == META_OBJ);
  CHECK_PROPERTY(o->contents == nullptr);
  CHECK_PROPERTY(o->num_regions == 0);
  CHECK_PROPERTY(o->regions == nullptr);
  CHECK_PROPERTY(o->walls == nullptr);
  CHECK_PROPERTY(o->wall_p == nullptr);
  CHECK_PROPERTY(o->vertices == nullptr);
  CHECK_PROPERTY(o->total_area == 0);
  CHECK_PROPERTY(o->n_tiles == 0);
  CHECK_PROPERTY(o->n_occupied_tiles == 0);
  CHECK_PROPERTY(o->n_occupied_tiles == 0);
  CHECK_PROPERTY(t_matrix_to_mat4x4(o->t_matrix) == mat4x4(1) && "only identity matrix for now");
  // root->is_closed - not checked
  CHECK_PROPERTY(o->periodic_x == false);
  CHECK_PROPERTY(o->periodic_y == false);
  CHECK_PROPERTY(o->periodic_z == false);
  return true;
}

bool mcell3_world_converter::convert_geometry_objects(volume* s) {

  object* root = s->root_instance;
  CHECK_PROPERTY(check_meta_object(root, "WORLD_INSTANCE"));
  CHECK_PROPERTY(root->first_child == root->last_child && "Only one scene expected");
  object* scene = root->first_child;
  CHECK_PROPERTY(check_meta_object(scene, "Scene"));

  object* curr_obj = scene->first_child;
  while (curr_obj != nullptr) {
    if (curr_obj->object_type == POLY_OBJ) {
      convert_polygonal_object(curr_obj);
    }
    else if (curr_obj->object_type == REL_SITE_OBJ) {
      // ignored
    }
    else {
      CHECK_PROPERTY(false && "Unexpected type of object");
    }

    curr_obj = curr_obj->next;
  }

  return true;
}


bool mcell3_world_converter::convert_wall(
    wall* w,
    wall_t& res_wall,
    std::vector<partition_vertex_index_pair_t>& res_vertices
) {
  CHECK_PROPERTY(w->surf_class_head == nullptr); // for now
  CHECK_PROPERTY(w->num_surf_classes == 0); // for now

  res_wall.side = w->side;

  for (int i = 0; i < 3; i++) {
    // this vertex was inserted into the same partition as the whole object
    res_vertices[i] =  get_mcell4_vertex_index(w->vert[i]);
  }
  res_wall.uv_vert1_u = w->uv_vert1_u;
  res_wall.uv_vert2 = w->uv_vert2;
  // struct edge *edges[3]; - ignored
  // struct wall *nb_walls[3]; - ignored
  // double area; - ignored
  res_wall.normal = w->normal;
  res_wall.unit_u = w->unit_u;
  res_wall.unit_v = w->unit_v;
  res_wall.distance_to_origin = w->d;
  CHECK_PROPERTY(w->grid == nullptr); // for now
  CHECK_PROPERTY(w->flags == 4096); // not sure yet what flags are there for walls
  struct object *parent_object; // set when this object is added
  // struct storage *birthplace; - ignored
  // struct region_list *counting_regions; // TODO, not sure

  return true;
}


bool mcell3_world_converter::convert_polygonal_object(object* o) {
  geometry_object_t obj;

  // --- object ---

  // o->next - ignored
  // o->parent - ignored
  CHECK_PROPERTY(o->first_child == nullptr);
  CHECK_PROPERTY(o->last_child == nullptr);
  obj.name = get_sym_name(o->sym);
  // o->last_name - ignored
  CHECK_PROPERTY(o->object_type == POLY_OBJ);
  CHECK_PROPERTY(o->contents != nullptr); // ignored for now, not sure what is contained
  CHECK_PROPERTY(o->num_regions == 1); // for now

  //TODO: o->regions // this probably contains some flags and other data

  CHECK_PROPERTY(o->n_walls == o->n_walls_actual); // ignored
  CHECK_PROPERTY(o->walls == nullptr); // this is null for some reason
  CHECK_PROPERTY(o->wall_p != nullptr);

  // --- vertices ---
  // to stay identical to mcell3, will use the exact number of vertices as in mcell3, for this to work,
  // vector_ptr_to_vertex_index_map is a 'global' map for the whole conversion process
  // one of the reasons to not to copy vertex coordinates is that they are shared among triangles of an object
  // and when we move one vertex of the object, we transform all the triangles (walls) that use it
  for (int i = 0; i < o->n_verts; i++) {
    // insert vertex into the right parition and returns partition index and vertex index
    partition_vertex_index_pair_t vertex_info = world->add_geometry_vertex(*o->vertices[i]);

    // check that if we are adding a vertex, it is exactly the same as there was before
    auto it = vector_ptr_to_vertex_index_map.find(o->vertices[i]);
    if (it != vector_ptr_to_vertex_index_map.end()) {
      // note: this check probably doesn't make sense because the mcell3 vertices
      // would have to change during conversion
      assert(it->second == vertex_info);
    }
    else {
      vector_ptr_to_vertex_index_map[o->vertices[i]] = vertex_info;
    }
  }

  // --- walls ---
  vector<wall_t> walls;
  // vertex info contains also partition indices when it is inserted into the
  // world geometry
  vector< vector<partition_vertex_index_pair_t> > walls_vertices;

  walls.resize(o->n_walls);
  walls_vertices.resize(o->n_walls);
  for (int i = 0; i < o->n_walls; i++) {
    walls_vertices[i].resize(VERTICES_IN_TRIANGLE);
    // uses precomputed map vector_ptr_to_vertex_index_map to transfrom vertices
    convert_wall(o->wall_p[i], walls[i], walls_vertices[i]);
  }

  // --- back to object ---

  CHECK_PROPERTY(o->n_tiles == 0);
  CHECK_PROPERTY(o->n_occupied_tiles == 0);
  CHECK_PROPERTY(o->n_occupied_tiles == 0);
  CHECK_PROPERTY(t_matrix_to_mat4x4(o->t_matrix) == mat4x4(1) && "only identity matrix for now");
  // root->is_closed - not checked
  CHECK_PROPERTY(o->periodic_x == false);
  CHECK_PROPERTY(o->periodic_y == false);
  CHECK_PROPERTY(o->periodic_z == false);

  // sets all ids and also wall_indices for the object
  world->add_geometry_object(obj, walls, walls_vertices);

  return true;
}

// cannot fail
void mcell3_world_converter::create_diffusion_events() {
  assert(!world->species.empty() && "There must be at least 1 species");

  set<float_t> time_steps_set;
  for (auto &species : world->species ) {
    time_steps_set.insert(species.time_step);
  }

  for (float_t time_step : time_steps_set) {
    diffuse_react_event_t* event = new diffuse_react_event_t(world, time_step);
    event->event_time = TIME_SIMULATION_START;
    world->scheduler.schedule_event(event);
  }
}


bool mcell3_world_converter::convert_species_and_create_diffusion_events(volume* s) {
  // TODO: many items are not checked
  for (int i = 0; i < s->n_species; i++) {
    species* spec = s->species_list[i];
    // not sure what to do with these superclasses
    if (spec == s->all_mols || spec == s->all_volume_mols || spec == s->all_surface_mols) {
      continue;
    }

    species_t new_species;

    new_species.species_id = world->species.size(); // id corresponds to the index in the species array
    new_species.mcell3_species_id = spec->species_id;
    new_species.D = spec->D;
    new_species.name = get_sym_name(spec->sym);
    new_species.space_step = spec->space_step;
    new_species.time_step = spec->time_step;
    CHECK_PROPERTY(spec->flags == 0 || spec->flags == SPECIES_FLAG_CAN_VOLVOL);
    new_species.flags = spec->flags;

    world->species.push_back(new_species);

    mcell3_species_id_map[new_species.mcell3_species_id] = new_species.species_id;
  }

  create_diffusion_events();

  return true;
}


bool mcell3_world_converter::convert_single_reaction(rxn *rx) {
  world->reactions.push_back(reaction_t());
  reaction_t& reaction = world->reactions.back();

  // rx->next - handled in convert_reactions
  // rx->sym->name - ignored, name obtained from pathway

  //?? u_int n_reactants - obtained from pthways, might check it

  CHECK_PROPERTY(rx->n_pathways == 1); // limited for now
  assert(rx->cum_probs != nullptr);
  // ?? reaction.cum_prob = rx->cum_probs[0]; - what is this good for?

  CHECK_PROPERTY(rx->cum_probs[0] == rx->max_fixed_p); // limited for now
  CHECK_PROPERTY(rx->cum_probs[0] == rx->min_noreaction_p); // limited for now
  reaction.max_fixed_p = rx->max_fixed_p;
  reaction.min_noreaction_p = rx->min_noreaction_p;

  // ?? double pb_factor; /* Conversion factor from rxn rate to rxn probability (used for cooperativity) */

  // int *product_idx_aux - ignored, post-processing information
  // u_int *product_idx - ignored, post-processing information
  // struct species **players - ignored, might check it but will contain the same info as pathways

  // NFSIM struct species ***nfsim_players; /* a matrix of the nfsim elements associated with each path */
  // NFSIM short *geometries;         /* Geometries of reactants/products */
  // NFSIM short **nfsim_geometries;   /* geometries of the nfsim geometries associated with each path */

  CHECK(rx->n_occurred == 0);
  CHECK(rx->n_skipped == 0);
  CHECK(rx->prob_t == nullptr);

  // TODO: pathway_info *info - magic_list, also some checks might be useful

  // --- pathway ---
  pathway *pathway_head = rx->pathway_head;
  CHECK(pathway_head->next == nullptr); // only 1 supported now

  reaction.name = pathway_head->pathname->sym->name;
  reaction.rate_constant = pathway_head->km;

  CHECK(pathway_head->orientation1 == 0 || pathway_head->orientation1 == 1 || pathway_head->orientation1 == -1);
  CHECK(pathway_head->orientation2 == 0 || pathway_head->orientation2 == 1 || pathway_head->orientation2 == -1);
  CHECK(pathway_head->orientation3 == 0 || pathway_head->orientation3 == 1 || pathway_head->orientation3 == -1);

  if (pathway_head->reactant1 != nullptr) {
    species_id_t reactant1_id = get_mcell4_species_id(pathway_head->reactant1->species_id);
    reaction.reactants.push_back(species_with_orientation_t(reactant1_id, pathway_head->orientation1));

    if (pathway_head->reactant2 != nullptr) {
      species_id_t reactant2_id = get_mcell4_species_id(pathway_head->reactant2->species_id);
      reaction.reactants.push_back(species_with_orientation_t(reactant2_id, pathway_head->orientation2));

      if (pathway_head->reactant3 != nullptr) {
        species_id_t reactant3_id = get_mcell4_species_id(pathway_head->reactant3->species_id);
        reaction.reactants.push_back(species_with_orientation_t(reactant3_id, pathway_head->orientation3));
      }
    }
    else {
      // reactant3 must be null if reactant2 is null
      assert(pathway_head->reactant3 == nullptr);
    }
  }
  else {
    assert(false && "No reactants?");
  }

  CHECK(pathway_head->km_filename == nullptr);

  // products
  product *product_ptr = pathway_head->product_head;
  while (product_ptr != nullptr) {
    species_id_t product_id = get_mcell4_species_id(product_ptr->prod->species_id);
    CHECK(product_ptr->orientation == 0 || product_ptr->orientation == 1 || product_ptr->orientation == -1);

    reaction.products.push_back(species_with_orientation_t(product_id, product_ptr->orientation));

    product_ptr = product_ptr->next;
  }

  CHECK(pathway_head->flags == 0);

  return true;
}


bool mcell3_world_converter::convert_reactions(volume* s) {

  rxn** reaction_hash = s->reaction_hash;
  int count = s->rx_hashsize;

  for (int i = 0; i < count; ++i) {
    rxn *rx = reaction_hash[i];
    while (rx != nullptr) {
      convert_single_reaction(rx);
      rx = rx->next;
    }
  }

  return true;
}


bool mcell3_world_converter::convert_release_events(volume* s) {

  // -- schedule_helper -- (as volume.releaser)
  schedule_helper* releaser = s->releaser;

  CHECK_PROPERTY(releaser->next_scale == nullptr);
  CHECK_PROPERTY(releaser->dt == 1);
  CHECK_PROPERTY(releaser->dt_1 == 1);
  CHECK_PROPERTY(releaser->now == 0);
  //ok now: CHECK_PROPERTY(releaser->count == 1);
  CHECK_PROPERTY(releaser->index == 0);

  for (int i = -1; i < releaser->buf_len; i++) {
    for (abstract_element *aep = (i < 0) ? releaser->current : releaser->circ_buf_head[i];
         aep != NULL; aep = aep->next) {

      release_event_t* event = new release_event_t(world);

      // -- release_event_queue --
      release_event_queue *req = (release_event_queue *)aep;

      event->event_time = req->event_time;

      // -- release_site --
      release_site_obj* rel_site = req->release_site;

      assert(rel_site->location != nullptr);
      event->location = vec3_t(*rel_site->location);
      event->species_id = get_mcell4_species_id(rel_site->mol_type->species_id);

      CHECK_PROPERTY(rel_site->release_number_method == 0);
      event->release_shape = rel_site->release_shape;
      CHECK_PROPERTY(rel_site->orientation == 0);

      event->release_number = rel_site->release_number;

      CHECK_PROPERTY(rel_site->mean_diameter == 0); // temporary
      CHECK_PROPERTY(rel_site->concentration == 0); // temporary
      CHECK_PROPERTY(rel_site->standard_deviation == 0); // temporary
      assert(rel_site->diameter != nullptr);
      event->diameter = *rel_site->diameter; // temporary
      CHECK_PROPERTY(rel_site->region_data == nullptr); // temporary?
      CHECK_PROPERTY(rel_site->mol_list == nullptr);
      CHECK_PROPERTY(rel_site->release_prob == 1); // temporary
      // rel_site->periodic_box - ignoring?
      // rel_site->pattern - TODO - is not null
      event->name = rel_site->name;
      // rel_site->graph_pattern - TODO - is not null - NFSim?

      // -- release_event_queue -- (again)
      CHECK_PROPERTY(t_matrix_to_mat4x4(req->t_matrix) == mat4x4(1) && "only identity matrix for now");
      CHECK_PROPERTY(req->train_counter == 0);
      CHECK_PROPERTY(req->train_high_time == 0);

      world->scheduler.schedule_event(event);
    }
  }

  // -- schedule_helper -- (again)
  CHECK_PROPERTY(releaser->current_count ==  0);
  CHECK_PROPERTY(releaser->current == nullptr);
  CHECK_PROPERTY(releaser->current_tail == nullptr);
  CHECK_PROPERTY(releaser->defunct_count == 0);
  CHECK_PROPERTY(releaser->error == 0);
  CHECK_PROPERTY(releaser->depth == 0);

  return true;
}


bool mcell3_world_converter::convert_viz_output_events(volume* s) {

  // -- viz_output_block --
  viz_output_block* viz_blocks = s->viz_blocks;
  if (viz_blocks == nullptr) {
    return true; // no visualization data
  }
  CHECK_PROPERTY(viz_blocks->next == nullptr);
  CHECK_PROPERTY(viz_blocks->viz_mode == NO_VIZ_MODE || viz_blocks->viz_mode  == ASCII_MODE || viz_blocks->viz_mode == CELLBLENDER_MODE); // just checking valid values
  viz_mode_t viz_mode = viz_blocks->viz_mode;
  const char* file_prefix_name = world->add_const_string_to_pool(viz_blocks->file_prefix_name);
  CHECK_PROPERTY(viz_blocks->viz_output_flag == VIZ_ALL_MOLECULES); // limited for now, TODO
  CHECK_PROPERTY(viz_blocks->species_viz_states != nullptr && (*viz_blocks->species_viz_states == (int)0x80000000 || *viz_blocks->species_viz_states == 0x7FFFFFFF)); // TODO: not sure what this means
  CHECK_PROPERTY(viz_blocks->default_mol_state == 0x7FFFFFFF); // not sure what this means

  // -- frame_data_head --
  frame_data_list* frame_data_head = viz_blocks->frame_data_head;
  CHECK_PROPERTY(frame_data_head->next == nullptr);
  CHECK_PROPERTY(frame_data_head->list_type == OUTPUT_BY_ITERATION_LIST); // limited for now
  CHECK_PROPERTY(frame_data_head->type == ALL_MOL_DATA); // limited for now
  CHECK_PROPERTY(frame_data_head->viz_iteration == 0); // must be zero at sim. start

  num_expr_list* iteration_ptr = frame_data_head->iteration_list;
  num_expr_list* curr_viz_iteration_ptr = frame_data_head->curr_viz_iteration;
  for (long long i = 0; i < frame_data_head->n_viz_iterations; i++) {
    assert(iteration_ptr != nullptr);
    assert(curr_viz_iteration_ptr != nullptr);
    assert(iteration_ptr->value == curr_viz_iteration_ptr->value);

    // create an event for each iteration
    viz_output_event_t* event = new viz_output_event_t(world);
    event->event_time = iteration_ptr->value;
    event->viz_mode = viz_mode;
    event->file_prefix_name = file_prefix_name;

    world->scheduler.schedule_event(event);

    iteration_ptr = iteration_ptr->next;
    curr_viz_iteration_ptr = curr_viz_iteration_ptr->next;
  }

  return true;
}


} // namespace mcell
