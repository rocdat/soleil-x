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




void create_field_pointer(PhysicalRegion region,
                          FieldData *&field,
                          int fieldID,
                          ByteOffset stride[3],
                          Runtime *runtime,
                          Context context) {
  
  Domain indexSpaceDomain = runtime->get_index_space_domain(context, region.get_logical_region().get_index_space());
  Rect<3> bounds = indexSpaceDomain.get_rect<3>();
  RegionAccessor<AccessorType::Generic, FieldData> acc = region.get_field_accessor(fieldID).typeify<FieldData>();
  Rect<3> tempBounds;
  field = acc.raw_rect_ptr<3>(bounds, tempBounds, stride);
  assert(bounds == tempBounds);
}


void create_field_pointer(PhysicalRegion region,
                          int *&field,
                          int fieldID,
                          ByteOffset stride[3],
                          Runtime *runtime,
                          Context context) {
  
  Domain indexSpaceDomain = runtime->get_index_space_domain(context, region.get_logical_region().get_index_space());
  Rect<3> bounds = indexSpaceDomain.get_rect<3>();
  RegionAccessor<AccessorType::Generic, int> acc = region.get_field_accessor(fieldID).typeify<int>();
  Rect<3> tempBounds;
  field = acc.raw_rect_ptr<3>(bounds, tempBounds, stride);
  assert(bounds == tempBounds);
}


void accessCellData(legion_physical_region_t *cells,
                    legion_field_id_t *cells_fields,
                    FieldData* &velocity,
                    FieldData* &centerCoordinates,
                    FieldData* &temperature,
                    ByteOffset strideCenter[3],
                    ByteOffset strideVelocity[3],
                    ByteOffset strideTemperature[3],
                    Rect<3> &bounds,
                    Runtime* runtime,
                    Context ctx) {
  
  PhysicalRegion* cell = CObjectWrapper::unwrap(cells[0]);
  std::vector<legion_field_id_t> fields;
  cell->get_fields(fields);
  
  for(unsigned field = 0; field < fields.size(); ++field) {
    PhysicalRegion* cell = CObjectWrapper::unwrap(cells[field]);
    Domain indexSpaceDomain = runtime->get_index_space_domain(ctx, cell->get_logical_region().get_index_space());
    bounds = indexSpaceDomain.get_rect<3>();
    
    switch(field) {
      case 0:
        create_field_pointer(*cell, velocity, cells_fields[field], strideVelocity, runtime, ctx);
        break;
        
      case 1:
        create_field_pointer(*cell, centerCoordinates, cells_fields[field], strideCenter, runtime, ctx);
        break;
        
      case 2:
        create_field_pointer(*cell, temperature, cells_fields[field], strideTemperature, runtime, ctx);
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
  
  std::ofstream outputFile;
  outputFile.open(filePath);
  outputFile << bounds << std::endl;
  int counter = 0;
  
  for(coord_t z = bounds.lo.x[2]; z < bounds.hi.x[2]; ++z) {
    for(coord_t y = bounds.lo.x[1]; y < bounds.hi.x[1]; ++y) {
      for(coord_t x = bounds.lo.x[0]; x < bounds.hi.x[0]; ++x) {
        outputFile
        << counter++ << " "
        << "(" << x << "," << y << "," << z << ") "
        << std::setprecision(10)
        << centerCoordinates[0] << " " << centerCoordinates[1] << " " << centerCoordinates[2] << "  "
        << velocity[0] << " " << velocity[1] << " " << velocity[2] << "  "
        << temperature[0]
        << std::endl;
        centerCoordinates += strideCenter[0].offset / sizeof(FieldData);
        velocity += strideVelocity[0].offset / sizeof(FieldData);
        temperature += strideTemperature[0].offset / sizeof(FieldData);
      }
    }
  }
  outputFile.close();
}



void create_field_pointer2(PhysicalRegion region,
                           int *&field,
                           int fieldID,
                           ByteOffset stride[1],
                           Runtime *runtime,
                           Context context) {
  
  Domain indexSpaceDomain = runtime->get_index_space_domain(context, region.get_logical_region().get_index_space());
  Rect<3> bounds = indexSpaceDomain.get_rect<3>();
  RegionAccessor<AccessorType::Generic, int> acc = region.get_field_accessor(fieldID).typeify<int>();
  Rect<3> tempBounds;
  field = acc.raw_rect_ptr<3>(bounds, tempBounds, stride);
  assert(bounds == tempBounds);
}


void getBaseIndexSpaceInt(PhysicalRegion* particle, int fieldID, int* &base, IndexSpace &indexSpace) {
  RegionAccessor<AccessorType::Generic, int> acc = particle->get_field_accessor(fieldID).typeify<int>();
  void* b = NULL;
  size_t stride = 0;
  acc.get_soa_parameters(b, stride);
  base = (int*)b;
  assert(stride == sizeof(int));
  indexSpace = particle->get_logical_region().get_index_space();
}


void getBaseIndexSpaceFloat_impl(PhysicalRegion* particle, int fieldID, FieldData* &base, IndexSpace &indexSpace, int numFields) {
  RegionAccessor<AccessorType::Generic, FieldData> acc = particle->get_field_accessor(fieldID).typeify<FieldData>();
  void* b = NULL;
  size_t stride = 0;
  acc.get_soa_parameters(b, stride);
  base = (FieldData*)b;
  assert(stride == sizeof(FieldData) * numFields);
  indexSpace = particle->get_logical_region().get_index_space();
}

void getBaseIndexSpaceFloat(PhysicalRegion* particle, int fieldID, FieldData* &base, IndexSpace &indexSpace) {
  getBaseIndexSpaceFloat_impl(particle, fieldID, base, indexSpace, 1);
}



void getBaseIndexSpaceFloat3(PhysicalRegion* particle, int fieldID, FieldData* &base, IndexSpace &indexSpace) {
  getBaseIndexSpaceFloat_impl(particle, fieldID, base, indexSpace, 3);
}



void accessParticleData(legion_physical_region_t *particles,
                        legion_field_id_t *particles_fields,
                        int* &cellX,
                        IndexSpace &cellXIS,
                        int* &cellY,
                        IndexSpace &cellYIS,
                        int* &cellZ,
                        IndexSpace &cellZIS,
                        FieldData* &position,
                        IndexSpace &positionIS,
                        FieldData* &density,
                        IndexSpace &densityIS,
                        FieldData* &particleTemperature,
                        IndexSpace &particleTemperatureIS,
                        Runtime* runtime,
                        Context ctx) {
  
  PhysicalRegion* particle = CObjectWrapper::unwrap(particles[0]);
  std::vector<legion_field_id_t> fields;
  particle->get_fields(fields);
  
  for(unsigned field = 0; field < fields.size(); ++field) {
    PhysicalRegion* particle = CObjectWrapper::unwrap(particles[field]);
    
    switch(field) {
      case 0:
        getBaseIndexSpaceInt(particle, particles_fields[field], cellX, cellXIS);
        break;
        
      case 1:
        getBaseIndexSpaceInt(particle, particles_fields[field], cellY, cellYIS);
        break;
        
      case 2:
        getBaseIndexSpaceInt(particle, particles_fields[field], cellZ, cellZIS);
        break;
        
      case 3:
        getBaseIndexSpaceFloat3(particle, particles_fields[field], position, positionIS);
        break;
        
      case 4:
        getBaseIndexSpaceFloat(particle, particles_fields[field], density, densityIS);
        break;
        
      case 5:
        getBaseIndexSpaceFloat(particle, particles_fields[field], particleTemperature, particleTemperatureIS);
        break;
        
      default:
        std::cerr << "oops, field not found" << std::endl;
    }
  }
}


static inline long long getNextInt(IndexIterator iterator) {
  return iterator.next().value;
}

void writeParticlesToFile(std::string filePath,
                          int* cellXBase,
                          IndexSpace cellXIS,
                          int* cellYBase,
                          IndexSpace cellYIS,
                          int* cellZBase,
                          IndexSpace cellZIS,
                          FieldData* positionBase,
                          IndexSpace positionIS,
                          FieldData* densityBase,
                          IndexSpace densityIS,
                          FieldData* particleTemperatureBase,
                          IndexSpace particleTemperatureIS,
                          Runtime* runtime,
                          Context ctx) {
  
  std::ofstream outputFile;
  outputFile.open(filePath);
  int counter = 0;
  
  IndexIterator cellXIterator(runtime, ctx, cellXIS);
  IndexIterator cellYIterator(runtime, ctx, cellYIS);
  IndexIterator cellZIterator(runtime, ctx, cellZIS);
  IndexIterator positionIterator(runtime, ctx, positionIS);
  IndexIterator densityIterator(runtime, ctx, densityIS);
  IndexIterator particleTemperatureIterator(runtime, ctx, particleTemperatureIS);
  
  while(cellXIterator.has_next()) {
#define NEXT(FIELD) (*(FIELD##Base + FIELD##Iterator.next().value))
    int cellX = NEXT(cellX);
    int cellY = NEXT(cellY);
    int cellZ = NEXT(cellZ);
    FieldData positionX = NEXT(position);
    FieldData positionY = NEXT(position);
    FieldData positionZ = NEXT(position);
    FieldData density = NEXT(density);
    FieldData particleTemperature = NEXT(particleTemperature);
    if(cellX > 0 && cellY > 0 && cellZ > 0) {
      std::cout << std::setprecision(10)
      << "(" << cellX << "," << cellY << "," << cellZ << ") "
      << positionX << " " << positionY << " " << positionZ << "  "
      << density << " "
      << particleTemperature
      << std::endl;
      counter++;
    }
  }
  
  outputFile.close();
  std::cout << "wrote " << counter << " particles to " << filePath << std::endl;
}




int timeStep = 0;//to do pass this in


std::string dataFileName(std::string table, int timeStep, Rect<3> bounds) {
  char buffer[256];
  sprintf(buffer, "%s.%d.%lld_%lld_%lld__%lld_%lld_%lld.txt",
          table.c_str(), timeStep,
          bounds.lo.x[0], bounds.lo.x[1], bounds.lo.x[2],
          bounds.hi.x[0], bounds.hi.x[1], bounds.hi.x[2]);
  return std::string(buffer);
}


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
  
  FieldData* centerCoordinates = NULL;
  FieldData* velocity = NULL;
  FieldData* temperature = NULL;
  ByteOffset strideCenter[3];
  ByteOffset strideVelocity[3];
  ByteOffset strideTemperature[3];
  Rect<3> bounds;
  
  accessCellData(cells, cells_fields, centerCoordinates, velocity, temperature,
                 strideCenter, strideVelocity, strideTemperature, bounds, runtime, ctx);
  std::string cellsFileName = dataFileName("./out/cells", timeStep / 4, bounds);
  writeCellsToFile(cellsFileName, bounds, velocity, centerCoordinates,
                   temperature, strideCenter, strideVelocity, strideTemperature);
  std::cout << cellsFileName << std::endl;
  
  
  int* cellX = NULL;
  IndexSpace cellXIS;
  int* cellY = NULL;
  IndexSpace cellYIS;
  int* cellZ = NULL;
  IndexSpace cellZIS;
  FieldData* position = NULL;
  IndexSpace positionIS;
  FieldData* density = NULL;
  IndexSpace densityIS;
  FieldData* particle_temperature = NULL;
  IndexSpace particle_temperatureIS;
  
  accessParticleData(particles, particles_fields, cellX, cellXIS, cellY, cellYIS,
                     cellZ, cellZIS, position, positionIS, density, densityIS,
                     particle_temperature, particle_temperatureIS, runtime, ctx);
  std::string particlesFileName = dataFileName("./out/particles", timeStep / 4, bounds);
  writeParticlesToFile(particlesFileName, cellX, cellXIS, cellY, cellYIS,
                       cellZ, cellZIS, position, positionIS, density, densityIS,
                       particle_temperature, particle_temperatureIS, runtime, ctx);
  std::cout << particlesFileName << std::endl;
  
  timeStep++;
  
}
