/* Copyright 2016 Stanford University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef __VIZ_H__
#define __VIZ_H__


typedef double FieldData;
typedef float PixelField;

#include "legion_c.h"

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include "OpenGL/glu.h"
#else
#include "GL/osmesa.h"
#include "GL/glu.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif
  
  void cxx_render(legion_runtime_t runtime,
                  legion_context_t context,
                  legion_physical_region_t *cells,
                  legion_field_id_t *cells_fields,
                  legion_physical_region_t *particles,
                  legion_field_id_t *particles_fields,
                  legion_physical_region_t *imageRegion,
                  legion_field_id_t *imageRegion_fields,
                  int xnum, int ynum, int znum);
  
  void cxx_reduce(legion_runtime_t runtime,
                  legion_context_t context,
                  legion_physical_region_t *imageSubregion,
                  legion_field_id_t *imageSubregion_fields,
                  int treeLevel,
                  int offset);
  

#ifdef __cplusplus
}
#endif



#endif // __VIZ_H__

