#ifndef __RENDER_H__
#define __RENDER_H__

#include "legion_c.h"

#ifdef __cplusplus
extern "C" {
#endif

void cxx_render(legion_runtime_t runtime,
                legion_context_t context,
                legion_physical_region_t *cells,
                legion_field_id_t *cells_fields,
                legion_physical_region_t *particles,
                legion_field_id_t *particles_fields,
                int xnum, int ynum, int znum);

#ifdef __cplusplus
}
#endif

#endif // __RENDER_H__

