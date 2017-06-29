#include "legion_c.h"
#include "legion_c_util.h"
#include "render.h"

using namespace Legion;
using namespace LegionRuntime::Arrays;

void cxx_render(legion_runtime_t runtime_,
                legion_context_t ctx_,
                legion_physical_region_t *cells,
                legion_field_id_t *cells_fields,
                legion_physical_region_t *particles,
                legion_field_id_t *particles_fields)
{
  Runtime *runtime = CObjectWrapper::unwrap(runtime_);
  Context ctx = CObjectWrapper::unwrap(ctx_)->context();
  
  std::cout << "Cells" << std::endl;
  std::vector<PhysicalRegion *> cells_regions;
  for (unsigned i = 0; i < 1; i++) {
    PhysicalRegion* cell = CObjectWrapper::unwrap(cells[i]);
    Domain indexSpaceDomain = runtime->get_index_space_domain(ctx, cell->get_logical_region().get_index_space());
    Rect<3> bounds = indexSpaceDomain.get_rect<3>();
    std::cout << i << " " << bounds << std::endl;
    cells_regions.push_back(cell);
    std::vector<FieldID> fields;
    cell->get_fields(fields);
    std::cout << fields.size() << " fields: ";
    for(unsigned j = 0; j < fields.size(); ++j) {
      std::cout << fields[j] << " ";
    }
    std::cout << std::endl;
  }


  std::cout << "Particles" << std::endl;
  std::vector<PhysicalRegion *> particles_regions;
  for (unsigned i = 0; i < 5; i++) {
    PhysicalRegion* particle = CObjectWrapper::unwrap(particles[i]);
    Domain indexSpaceDomain = runtime->get_index_space_domain(ctx, particle->get_logical_region().get_index_space());
    std::cout << i << " " << indexSpaceDomain.get_dim() << " " << indexSpaceDomain << std::endl;
    particles_regions.push_back(particle);
    std::vector<FieldID> fields;
    particle->get_fields(fields);
    std::cout << fields.size() << " fields: ";
    for(unsigned j = 0; j < fields.size(); ++j) {
      std::cout << fields[j] << " ";
    }
    std::cout << std::endl;
  }

}
