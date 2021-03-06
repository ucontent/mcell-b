cmake_minimum_required(VERSION 3.4)
project(mcell)

add_definitions(-DPACKAGE_BUGREPORT="mcell@salk.edu")

SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

# directories holding flex/bison files and out of source includes.
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/deps)
include_directories("${CMAKE_SOURCE_DIR}/src" "${CMAKE_CURRENT_BINARY_DIR}/deps" "${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/src4")

# move config.h into place. Unix and Windows have different header files
if (UNIX)
  file(COPY ${CMAKE_SOURCE_DIR}/src/config-nix.h DESTINATION
    ${CMAKE_CURRENT_BINARY_DIR}/deps)
  file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/deps/config-nix.h
    ${CMAKE_CURRENT_BINARY_DIR}/deps/config.h)
elseif (WIN32)
  file(COPY ${CMAKE_SOURCE_DIR}/src/config-win.h DESTINATION
    ${CMAKE_CURRENT_BINARY_DIR}/deps)
  file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/deps/config-win.h
    ${CMAKE_CURRENT_BINARY_DIR}/deps/config.h)
endif()

# check for bison/flex and set up code gen
find_package(BISON)
find_package(FLEX)
BISON_TARGET(mdlParser ${CMAKE_SOURCE_DIR}/src/mdlparse.y
  ${CMAKE_CURRENT_BINARY_DIR}/deps/mdlparse.c)

file(COPY ${CMAKE_SOURCE_DIR}/src/mdllex.l DESTINATION
  ${CMAKE_CURRENT_BINARY_DIR}/deps)

FLEX_TARGET(mdlScanner ${CMAKE_CURRENT_BINARY_DIR}/deps/mdllex.l
  ${CMAKE_CURRENT_BINARY_DIR}/deps/mdlex.c COMPILE_FLAGS -Crema)
ADD_FLEX_BISON_DEPENDENCY(mdlScanner mdlParser mdllex_l)

# create version.h target
if (NOT WIN32)
  add_custom_target(
    version_h
    COMMAND ${CMAKE_SOURCE_DIR}/src/version.sh > ${CMAKE_CURRENT_BINARY_DIR}/deps/version.h
    DEPENDS ${CMAKE_SOURCE_DIR}/src/version.sh
    VERBATIM)
elseif (WIN32)
  add_custom_target(version_h)
  add_custom_command(TARGET version_h COMMAND ${CMAKE_COMMAND} -E
                     copy ${CMAKE_SOURCE_DIR}/appveyor_windows/version.h ${CMAKE_CURRENT_BINARY_DIR}/deps )
endif()

set(OPTIMIZATION_FLAGS " -march=native -mavx2 -O3")
set(WARNING_FLAGS "-Wall -Wextra -Wshadow -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable")

set(CMAKE_C_FLAGS "-g3  -D_GNU_SOURCE=1 ${WARNING_FLAGS} ${OPTIMIZATION_FLAGS} -std=c11 ${CMAKE_C_FLAGS}" )
set(CMAKE_CXX_FLAGS "-g3 -D_GNU_SOURCE=1 -faligned-new ${WARNING_FLAGS} ${OPTIMIZATION_FLAGS} ${CMAKE_CXX_FLAGS}" )

if (NOT WIN32)
  # check for needed libraries
  find_library(M_LIB m)
  set(CMAKE_EXE_LINKER_FLAGS ${M_LIB})
elseif (WIN32)
  set(CMAKE_EXE_LINKER_FLAGS "-lm")
endif()

if (PROFILING STREQUAL "ON")
  set(CMAKE_C_FLAGS "-pg ${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "-pg ${CMAKE_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "-pg ${CMAKE_EXE_LINKER_FLAGS}")
endif()

set(SOURCE_FILES
    src/argparse.c
    src/chkpt.c
    src/count_util.c
    src/diffuse.c
    src/diffuse_trimol.c
    src/diffuse_util.c
    src/dyngeom.c
    src/dyngeom_lex.c
    src/dyngeom_parse_extras.c
    src/dyngeom_yacc.c
    src/grid_util.c
    src/hashmap.c
    src/init.c
    src/isaac64.c
    src/logging.c
    src/mcell.c
    src/mcell_dyngeom.c
    src/mcell_init.c
    src/mcell_misc.c
    src/mcell_objects.c
    src/mcell_react_out.c
    src/mcell_reactions.c
    src/mcell_release.c
    src/mcell_run.c
    src/mcell_species.c
    src/mcell_surfclass.c
    src/mcell_viz.c
    src/mdlparse.y
    src/dump_state.cpp
    src/mdlparse_util.c
    src/mem_util.c
    src/minrng.c
    src/nfsim_func.c
    src/react_cond.c
    src/react_outc.c
    src/react_outc_nfsim.c
    src/react_outc_trimol.c
    src/react_output.c
    src/react_trig.c
    src/react_trig_nfsim.c
    src/react_util.c
    src/react_util_nfsim.c
    src/rng.c
    src/sched_util.c
    src/strfunc.c
    src/sym_table.c
    src/test_api.c
    src/triangle_overlap.c
    src/util.c
    src/vector.c
    src/version_info.c
    src/viz_output.c
    src/vol_util.c
    src/volume_output.c
    src/wall_util.c
    
    src4/base_event.cpp
    src4/defines.cpp
    src4/diffuse_react_event.cpp
    src4/mcell3_world_converter.cpp
    src4/molecule.cpp
    src4/partition.cpp
    src4/reaction.cpp
    src4/release_event.cpp
    src4/scheduler.cpp
    src4/species.cpp
    src4/viz_output_event.cpp
    src4/defragmentation_event.cpp
    src4/geometry.cpp
    src4/world.cpp
    )

link_directories(${CMAKE_BINARY_DIR}/lib)
if (UNIX)
  SET(CMAKE_INSTALL_RPATH "$ORIGIN/./lib")
  SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()
FIND_PACKAGE(SWIG REQUIRED)
INCLUDE(${SWIG_USE_FILE})

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/src")

if (NOT WIN32)
  FIND_PACKAGE(PythonLibs 3 REQUIRED)
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

  #SET(CMAKE_SWIG_FLAGS "-debug-typemap;-debug-tmsearch;-debug-tmused;-ltypemaps")
  SET(CMAKE_SWIG_FLAGS "-ltypemaps;-py3")

  SET(CMAKE_SWIG_OUTDIR "${CMAKE_BINARY_DIR}/python")
  SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python")

  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/python)
  SET_SOURCE_FILES_PROPERTIES(src/pymcell.i PROPERTIES C ON)
  SET_SOURCE_FILES_PROPERTIES(src/pymcell.i PROPERTIES SWIG_FLAGS "-includeall")
  SWIG_ADD_MODULE(pymcell python
    src/argparse.c
    src/chkpt.c
    src/count_util.c
    src/diffuse.c
    src/diffuse_trimol.c
    src/diffuse_util.c
    src/dyngeom.c
    src/dyngeom_lex.c
    src/dyngeom_parse_extras.c
    src/dyngeom_yacc.c
    src/grid_util.c
    src/hashmap.c
    src/init.c
    src/isaac64.c
    src/logging.c
    src/mcell_dyngeom.c
    src/mcell_init.c
    src/mcell_misc.c
    src/mcell_objects.c
    src/mcell_react_out.c
    src/mcell_reactions.c
    src/mcell_release.c
    src/mcell_run.c
    src/mcell_species.c
    src/mcell_surfclass.c
    src/mcell_viz.c
    src/mem_util.c
    src/nfsim_func.c
    src/pymcell.i
    src/react_cond.c
    src/react_outc.c
    src/react_outc_nfsim.c
    src/react_outc_trimol.c
    src/react_output.c
    src/react_trig.c
    src/react_trig_nfsim.c
    src/react_util.c
    src/react_util_nfsim.c
    src/rng.c
    src/sched_util.c
    src/strfunc.c
    src/sym_table.c
    src/triangle_overlap.c
    src/util.c
    src/vector.c
    src/version_info.c
    src/viz_output.c
    src/vol_util.c
    src/volume_output.c
    src/wall_util.c)
  if (APPLE)
    SWIG_LINK_LIBRARIES(pymcell ${CMAKE_CURRENT_BINARY_DIR}/lib/libnfsim_c.dylib ${CMAKE_CURRENT_BINARY_DIR}/lib/libNFsim.dylib ${PYTHON_LIBRARIES})
  else()
    SWIG_LINK_LIBRARIES(pymcell ${CMAKE_CURRENT_BINARY_DIR}/lib/libnfsim_c.so ${CMAKE_CURRENT_BINARY_DIR}/lib/libNFsim.so ${PYTHON_LIBRARIES})
  endif()

  # copy the pyMCell test scripts into place
  file(GLOB PYMCELL_EXAMPLES "${CMAKE_SOURCE_DIR}/src/pymcell_examples/*")
  file(COPY ${PYMCELL_EXAMPLES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/python/)

  file(COPY ${CMAKE_SOURCE_DIR}/src/pymcell_unittests.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/python/)
endif()

# copy tools for MDLr to MDL
file(COPY ${CMAKE_SOURCE_DIR}/src/rules_py/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# copy tools for MDLr to MDL
file(COPY ${CMAKE_SOURCE_DIR}/bionetgen/bng2/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bng2/)

# build executable
add_executable(mcell
  ${SOURCE_FILES}
  src/mcell.c
  ${BISON_mdlParser_OUTPUTS}
  ${FLEX_mdlScanner_OUTPUTS})
target_link_libraries(mcell ${M_LIB} nfsim_c NFsim)

#set(CMAKE_EXE_LINKER_FLAGS "${M_LIB} -static-libgcc -static-libstdc++")

#add_executable(mcell_static
#  ${CMAKE_CURRENT_BINARY_DIR}/deps/version.h
#  ${SOURCE_FILES}
#  ${BISON_mdlParser_OUTPUTS}
#  ${FLEX_mdlScanner_OUTPUTS})
#target_link_libraries(mcell_static ${M_LIB} ${CMAKE_SOURCE_DIR}/lib/libnfsim_c_static.a ${CMAKE_SOURCE_DIR}/lib/libNFsim_static.a)

target_link_libraries(mcell ${M_LIB})
TARGET_COMPILE_DEFINITIONS(mcell PRIVATE NOSWIG=1)

# build little libmcell test
#add_executable(libmcell_test
#  ${CMAKE_CURRENT_BINARY_DIR}/deps/version.h
#  ${SOURCE_FILES}
#  src/libmcell_test.c
#  ${BISON_mdlParser_OUTPUTS}
#  ${FLEX_mdlScanner_OUTPUTS})
#target_link_libraries(libmcell_test ${M_LIB} nfsim_c NFsim)

# build nfsim and nfsimCInterface before trying to build MCell
add_custom_target(
  build_nfsim
  COMMAND python ${CMAKE_SOURCE_DIR}/requirements.py)
add_dependencies(mcell build_nfsim)
add_dependencies(mcell version_h)
if (NOT WIN32)
  add_dependencies(_pymcell build_nfsim)
  add_dependencies(_pymcell version_h)
endif()
