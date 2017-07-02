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


#include "psaap_render.cc"

#include "legion_c.h"
#include "legion_c_util.h"
#include "render.h"

#include <iostream>
#include <fstream>
#include <iomanip>


using namespace Legion;
using namespace LegionRuntime::Arrays;
using namespace LegionRuntime::Accessor;







static
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


#if 0

static
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

#endif


static
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

#ifdef WRITE_FILE

static
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

#endif


static
void getBaseIndexSpaceInt(PhysicalRegion* particle, int fieldID, int* &base, IndexSpace &indexSpace) {
  RegionAccessor<AccessorType::Generic, int> acc = particle->get_field_accessor(fieldID).typeify<int>();
  void* b = NULL;
  size_t stride = 0;
  acc.get_soa_parameters(b, stride);
  base = (int*)b;
  assert(stride == sizeof(int));
  indexSpace = particle->get_logical_region().get_index_space();
}


static
void getBaseIndexSpaceFloat_impl(PhysicalRegion* particle, int fieldID, FieldData* &base, IndexSpace &indexSpace, int numFields) {
  RegionAccessor<AccessorType::Generic, FieldData> acc = particle->get_field_accessor(fieldID).typeify<FieldData>();
  void* b = NULL;
  size_t stride = 0;
  acc.get_soa_parameters(b, stride);
  base = (FieldData*)b;
  assert(stride == sizeof(FieldData) * numFields);
  indexSpace = particle->get_logical_region().get_index_space();
}


static
void getBaseIndexSpaceFloat(PhysicalRegion* particle, int fieldID, FieldData* &base, IndexSpace &indexSpace) {
  getBaseIndexSpaceFloat_impl(particle, fieldID, base, indexSpace, 1);
}


static
void getBaseIndexSpaceFloat3(PhysicalRegion* particle, int fieldID, FieldData* &base, IndexSpace &indexSpace) {
  getBaseIndexSpaceFloat_impl(particle, fieldID, base, indexSpace, 3);
}


static
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



#ifdef WRITE_FILE

static
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
      outputFile << std::setprecision(10)
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

#endif



static int volatile timeStep = 0;//to do pass this in
const int NUM_NODES = 4;//TODO eliminate this when timeStep is passed in

static
std::string fileName(std::string table, std::string ext, int timeStep, Rect<3> bounds) {
  char buffer[256];
  sprintf(buffer, "%s.%d.%lld_%lld_%lld__%lld_%lld_%lld%s",
          table.c_str(),
          timeStep / NUM_NODES,//TODO don't divide by NUM_NODES once timeStep is passed in
          bounds.lo.x[0], bounds.lo.x[1], bounds.lo.x[2],
          bounds.hi.x[0], bounds.hi.x[1], bounds.hi.x[2],
          ext.c_str());
  return std::string(buffer);
}

#ifdef WRITE_FILE

std::string dataFileName(std::string table, int timeStep, Rect<3> bounds) {
  return fileName(table, ".txt", timeStep, bounds);
}

#endif


std::string imageFileName(std::string table, int timeStep, Rect<3> bounds) {
#ifdef IMAGE_FORMAT_TGA
  return fileName(table, ".tga", timeStep, bounds);
#else
  return fileName(table, ".ppm", timeStep, bounds);
#endif
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
  Rect<3> bounds = Rect<3>(Point<3>::ZEROES(), Point<3>::ZEROES());
  int numCells = 0;
  
  accessCellData(cells, cells_fields, centerCoordinates, velocity, temperature,
                 strideCenter, strideVelocity, strideTemperature, bounds, runtime, ctx);
  
#ifdef WRITE_FILE
  std::string cellsFileName = dataFileName("./out/cells", timeStep / 4, bounds);
  writeCellsToFile(cellsFileName, bounds, velocity, centerCoordinates,
                   temperature, strideCenter, strideVelocity, strideTemperature);
  std::cout << cellsFileName << std::endl;
#else
  int xLo = (int)bounds.lo.x[0];
  int yLo = (int)bounds.lo.x[1];
  int zLo = (int)bounds.lo.x[2];
  int xHi = (int)bounds.hi.x[0];
  int yHi = (int)bounds.hi.x[1];
  int zHi = (int)bounds.hi.x[2];
  numCells = (xHi - xLo) * (yHi - yLo) * (zHi - zLo);
#endif
  
  
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
  FieldData* particleTemperature = NULL;
  IndexSpace particleTemperatureIS;
  int numParticles = 0;
  
  accessParticleData(particles, particles_fields, cellX, cellXIS, cellY, cellYIS,
                     cellZ, cellZIS, position, positionIS, density, densityIS,
                     particleTemperature, particleTemperatureIS, runtime, ctx);
  
#ifdef WRITE_FILE
  std::string particlesFileName = dataFileName("./out/particles", timeStep / 4, bounds);
  writeParticlesToFile(particlesFileName, cellX, cellXIS, cellY, cellYIS,
                       cellZ, cellZIS, position, positionIS, density, densityIS,
                       particleTemperature, particleTemperatureIS, runtime, ctx);
  std::cout << particlesFileName << std::endl;
#else
  IndexIterator cellXIterator(runtime, ctx, cellXIS);
  while(cellXIterator.has_next()) {
    numParticles++;
    cellXIterator.next();
  }
#endif

#ifndef WRITE_FILE
  
  /* Create an RGBA-mode context */
#if OSMESA_MAJOR_VERSION * 100 + OSMESA_MINOR_VERSION >= 305
  /* specify Z, stencil, accum sizes */
  OSMesaContext mesaCtx = OSMesaCreateContextExt(GL_RGBA, 32, 0, 0, NULL);
#else
  OSMesaContext mesaCtx = OSMesaCreateContext(GL_RGBA, NULL);
#endif
  if (!mesaCtx) {
    printf("OSMesaCreateContext failed!\n");
    return;
  }

  GLfloat* rgbaBuffer = NULL;
  GLfloat* depthBuffer = NULL;
  const int width = 3840;
  const int height = 2160;
  
  render_image(width, height, centerCoordinates, velocity, temperature, numCells,
               position, density, particleTemperature, numParticles, &rgbaBuffer, &depthBuffer, mesaCtx);

#ifdef IMAGE_FORMAT_TGA
  write_targa(imageFileName("./out/image", timeStep, bounds).c_str(), rgbaBuffer, width, height);
#else
  write_ppm(imageFileName("./out/image", timeStep, bounds).c_str(), rgbaBuffer, width, height);
#endif
  
  /* free the image buffer */
  free(rgbaBuffer);
  free(depthBuffer);
  
  /* destroy the context */
  OSMesaDestroyContext(mesaCtx);

#endif
  
  timeStep++;
  
}
