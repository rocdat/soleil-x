#include "legion_c.h"
#include "legion_c_util.h"
#include "render.h"

#include <iostream>
#include <fstream>
#include <iomanip>


using namespace Legion;
using namespace LegionRuntime::Arrays;
using namespace LegionRuntime::Accessor;

typedef double FieldData;




void create_cell_field_pointer( PhysicalRegion cell,
                               FieldData *&field,
                               int fieldID,
                               ByteOffset stride[3],
                               Runtime *runtime,
                               Context context) {
  
  Domain indexSpaceDomain = runtime->get_index_space_domain(context, cell.get_logical_region().get_index_space());
  Rect<3> bounds = indexSpaceDomain.get_rect<3>();
  RegionAccessor<AccessorType::Generic, FieldData> acc = cell.get_field_accessor(fieldID).typeify<FieldData>();
  Rect<3> tempBounds;
  field = acc.raw_rect_ptr<3>(bounds, tempBounds, stride);
  assert(bounds == tempBounds);
}


void accessCellData(legion_physical_region_t *cells,
                    legion_field_id_t *cells_fields,
                    FieldData* &centerCoordinates,
                    FieldData* &velocity,
                    FieldData* &temperature,
                    ByteOffset strideCenter[3],
                    ByteOffset strideVelocity[3],
                    ByteOffset strideTemperature[3],
                    Rect<3> &bounds,
                    Runtime* runtime,
                    Context ctx) {
  const int NUM_CELL_FIELDS = 3;
  for(unsigned field = 0; field < NUM_CELL_FIELDS; ++field) {
    PhysicalRegion* cell = CObjectWrapper::unwrap(cells[field]);
    Domain indexSpaceDomain = runtime->get_index_space_domain(ctx, cell->get_logical_region().get_index_space());
    bounds = indexSpaceDomain.get_rect<3>();
    switch(field) {
      case 0:
        create_cell_field_pointer(*cell, centerCoordinates, cells_fields[field], strideCenter, runtime, ctx);
        break;
      case 1:
        create_cell_field_pointer(*cell, velocity, cells_fields[field], strideVelocity, runtime, ctx);
        break;
      case 2:
        create_cell_field_pointer(*cell, temperature, cells_fields[field], strideTemperature, runtime, ctx);
        break;
      default:
        std::cerr << "oops, field not found" << std::endl;
    }
  }
}



void writeCellsToFile(std::string filePath,
                      Rect<3> bounds,
                      FieldData* centerCoordinates,
                      FieldData* velocity,
                      FieldData* temperature,
                      ByteOffset strideCenter[3],
                      ByteOffset strideVelocity[3],
                      ByteOffset strideTemperature[3]) {
  std::cout << "strideCenter " << strideCenter[0].offset << "," << strideCenter[1].offset
  << "," << strideCenter[2].offset << std::endl;
  
  std::ofstream outputFile;
  outputFile.open(filePath);
  outputFile << bounds << std::endl;
  for (GenericPointInRectIterator<3> pir(bounds); pir; pir++) {
    outputFile << pir.p << " "
    << std::setprecision(20)
    << centerCoordinates[0] << " " << centerCoordinates[1] << " " << centerCoordinates[2] << " "
    << temperature[0] << " "
    << velocity[0] << " " << velocity[1] << " " << velocity[2]
    << std::endl;
    centerCoordinates += strideCenter[0].offset;
    velocity += strideVelocity[0].offset;
    temperature += strideTemperature[0].offset;
  }
  outputFile.close();
}


int timeStep = 0;//to do pass this in

void cxx_render(legion_runtime_t runtime_,
                legion_context_t ctx_,
                legion_physical_region_t *cells,
                legion_field_id_t *cells_fields,
                legion_physical_region_t *particles,
                legion_field_id_t *particles_fields,
                int xnum,
                int ynum,
                int znum)
{
  Runtime *runtime = CObjectWrapper::unwrap(runtime_);
  Context ctx = CObjectWrapper::unwrap(ctx_)->context();
  
  std::cout << "global bounds " << xnum << "," << ynum << "," << znum << std::endl;
  
  FieldData* centerCoordinates = NULL;
  FieldData* velocity = NULL;
  FieldData* temperature = NULL;
  ByteOffset strideCenter[3];
  ByteOffset strideVelocity[3];
  ByteOffset strideTemperature[3];
  Rect<3> bounds;
  
  accessCellData(cells, cells_fields, centerCoordinates, velocity, temperature,
                 strideCenter, strideVelocity, strideTemperature, bounds, runtime, ctx);
  
  
  char filename[256];
  sprintf(filename, "cells_%d__%lld_%lld_%lld__%lld_%lld_%lld.txt", timeStep++,
          bounds.lo.x[0], bounds.lo.x[1], bounds.lo.x[2],
          bounds.hi.x[0], bounds.hi.x[1], bounds.hi.x[2]);
  writeCellsToFile(std::string(filename), bounds, centerCoordinates, velocity,
                   temperature, strideCenter, strideVelocity, strideTemperature);
  
  std::cout << filename << std::endl;
  unsigned char* _d = (unsigned char*)centerCoordinates;
  printf("centerCoordinates %p = 0x%x %x %x %x %x %x %x %x = %lf\n",
         centerCoordinates, _d[0], _d[1], _d[2], _d[3], _d[4], _d[5], _d[6], _d[7], centerCoordinates[0]);

  
#if 0
  
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
#endif
  
}
