#include "legion_c.h"
#include "legion_c_util.h"
#include "render.h"

using namespace Legion;

void cxx_render(legion_runtime_t runtime_,
                legion_context_t ctx_,
                legion_physical_region_t *cells,
                legion_field_id_t *cells_fields,
                legion_physical_region_t *particles,
                legion_field_id_t *particles_fields)
{
  Runtime *runtime = CObjectWrapper::unwrap(runtime_);
  Context ctx = CObjectWrapper::unwrap(ctx_)->context();
  std::vector<PhysicalRegion *> cells_regions;
  for (int i = 0; i < 5; i++) {
    cells_regions.push_back(CObjectWrapper::unwrap(cells[i]));
  }
}
