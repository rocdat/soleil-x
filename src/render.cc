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


#include "legion_c.h"
#include "legion_c_util.h"
#include "render.h"

#include <iostream>
#include <fstream>
#include <iomanip>


using namespace Legion;
using namespace LegionRuntime::Arrays;
using namespace LegionRuntime::Accessor;


static const bool writeFiles = false;//write out text files with data

void
write_ppm(const char *filename, const GLfloat *rgba, int width, int height)
{
  FILE *f = fopen( filename, "w" );
  if (f) {
    printf ("writing ppm file %s\n", filename);
    int x, y;
    fprintf(f,"P6\n");
    fprintf(f,"# binary ppm file %s\n", filename);
    fprintf(f,"%i %i\n", width,height);
    fprintf(f,"255\n");
    fclose(f);
    
    f = fopen( filename, "ab" );  /* reopen in binary append mode */
    assert(f != NULL);
    for (y = height - 1; y >= 0; y--) {
      unsigned char outputBuffer[width * 3];
      unsigned char *outputPtr = outputBuffer;
      GLfloat* rgbaPtr = (GLfloat*)rgba + (y * width * 4);
      for (x = 0; x < width; x++) {
        int r, g, b;
        r = (int) (rgbaPtr[0] * 255.0);
        g = (int) (rgbaPtr[1] * 255.0);
        b = (int) (rgbaPtr[2] * 255.0);
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;
        outputPtr[0] = r;
        outputPtr[1] = g;
        outputPtr[2] = b;
        outputPtr += 3;
        rgbaPtr += 4;
      }
      fwrite(outputBuffer, 3 * sizeof(unsigned char), width, f);
    }
    fclose(f);
  } else {
    printf("could not write %s\n", filename);
  }
}




static void temperatureToColor(GLfloat temperature,
                               GLfloat color[4]) {
  
  // https://stackoverflow.com/questions/7229895/display-temperature-as-a-color-with-c
  
  float x = (float)(temperature / 1000.0);
  float x2 = x * x;
  float x3 = x2 * x;
  float x4 = x3 * x;
  float x5 = x4 * x;
  
  float R, G, B = 0.0f;
  
  // red
  if (temperature <= 6600)
    R = 1.0f;
  else
    R = 0.0002889f * x5 - 0.01258f * x4 + 0.2148f * x3 - 1.776f * x2 + 6.907f * x - 8.723f;
  
  // green
  if (temperature <= 6600)
    G = -4.593e-05f * x5 + 0.001424f * x4 - 0.01489f * x3 + 0.0498f * x2 + 0.1669f * x - 0.1653f;
  else
    G = -1.308e-07f * x5 + 1.745e-05f * x4 - 0.0009116f * x3 + 0.02348f * x2 - 0.3048f * x + 2.159f;
  
  // blue
  if (temperature <= 2000)
    B = 0.0f;
  else if (temperature < 6600)
    B = 1.764e-05f * x5 + 0.0003575f * x4 - 0.01554f * x3 + 0.1549f * x2 - 0.3682f * x + 0.2386f;
  else
    B = 1.0f;
  
  color[0] = R;
  color[1] = G;
  color[2] = B;
  color[3] = 1.0f;
}


static void scaledTemperatureToColor(GLfloat temperature,
                                     GLfloat color[2]) {
  const GLfloat min = 240.0f;
  const GLfloat max = 340.0f;
  const GLfloat Kmin = 0.0f;
  const GLfloat Kmax = 10000.0f;
  GLfloat scaledTemperature = (temperature - min) * ((Kmax - Kmin) / (max - min));
  return temperatureToColor(scaledTemperature, color);
}


static void drawVelocityVector(FieldData* centerCoordinate,
                               FieldData* velocity,
                               FieldData* temperature) {
  GLfloat scale = 0.05f;//TODO this is testcase dependent//TODO pass in domain bounds from simulation
  GLfloat base[] = {
    (GLfloat)centerCoordinate[0], (GLfloat)centerCoordinate[1], (GLfloat)centerCoordinate[2]
  };
  GLfloat v[] = {
    (GLfloat)velocity[0], (GLfloat)velocity[1], (GLfloat)velocity[2]
  };
  GLfloat top[] = {
    base[0] + v[0] * scale, base[1] + v[1] * scale, base[2] + v[2] * scale
  };
  GLfloat t = temperature[0];
  GLfloat color[4];
  scaledTemperatureToColor(t, color);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
  
  glVertex3fv(base);
  glVertex3fv(top);
}



static void drawParticle(GLUquadricObj* qobj, FieldData* position, FieldData* density, FieldData* particleTemperature) {
  
  GLfloat t = particleTemperature[0];
  GLfloat color[4];
  scaledTemperatureToColor(t, color);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
  
  glPushMatrix();
  const GLfloat verticalOffset = 0.5;//TODO this is testcase dependent
  glTranslatef(position[0], position[1], position[2] + verticalOffset);
  const float densityScale = 0.000001f;
  gluSphere(qobj, density[0] * densityScale, 10, 10);
  glPopMatrix();
}



static void drawParticles(bool* __validBase,
                          IndexSpace __validIS,
                          FieldData* positionBase,
                          IndexSpace positionIS,
                          FieldData* densityBase,
                          IndexSpace densityIS,
                          FieldData* particleTemperatureBase,
                          IndexSpace particleTemperatureIS,
                          Runtime* runtime,
                          Context ctx,
                          GLUquadricObj* qobj) {
  
  IndexIterator __validIterator(runtime, ctx, __validIS);
  IndexIterator positionIterator(runtime, ctx, positionIS);
  IndexIterator densityIterator(runtime, ctx, densityIS);
  IndexIterator particleTemperatureIterator(runtime, ctx, particleTemperatureIS);
  
  while(__validIterator.has_next()) {
#define NEXT(FIELD) (FIELD##Base + FIELD##Iterator.next().value)
#define NEXT3(FIELD) (FIELD##Base + FIELD##Iterator.next().value * 3)
    bool valid = *NEXT(__valid);
    if(valid) {
      FieldData* position = NEXT3(position);
      FieldData* density = NEXT(density);
      FieldData* particleTemperature = NEXT(particleTemperature);
      drawParticle(qobj, position, density, particleTemperature);
    }
  }
  
}


static void setCamera() {//TODO this is testcase dependent
  gluLookAt(/*eye*/20.0, 20.0, 10.0, /*at*/3.0, 3.0, 3.0, /*up*/0.0, 0.0, 1.0);
}




void render_image(int width,
                  int height,
                  FieldData* centerCoordinates,
                  FieldData* velocity,
                  FieldData* temperature,
                  int numCells,
                  bool* __valid,
                  IndexSpace __validIS,
                  FieldData* position,
                  IndexSpace positionIS,
                  FieldData* density,
                  IndexSpace densityIS,
                  FieldData* particleTemperature,
                  IndexSpace particleTemperatureIS,
                  GLfloat** rgbaBuffer,
                  GLfloat** depthBuffer,
                  OSMesaContext mesaCtx,
                  Runtime* runtime,
                  Context ctx)
{
  
  
  /* Allocate the image buffer */
  const int fieldsPerPixel = 4;
  *rgbaBuffer = (GLfloat *) malloc(width * height * fieldsPerPixel * sizeof(GLfloat));
  if (!*rgbaBuffer) {
    printf("Alloc image buffer failed!\n");
    return;
  }
  
  /* Bind the buffer to the context and make it current */
  if (!OSMesaMakeCurrent(mesaCtx, *rgbaBuffer, GL_FLOAT, width, height)) {
    printf("OSMesaMakeCurrent failed!\n");
    return;
  }
  
  
  
  
  GLfloat light_ambient[] = { 0.5, 0.5, 0.5, 1.0 };
  GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat light_position[] = { 0.0, 0.0, 50.0, 1.0 };
  //  GLfloat red_mat[]   = { 1.0, 0.2, 0.2, 1.0 };
  //  GLfloat green_mat[] = { 0.2, 1.0, 0.2, 0.5 };
  //  GLfloat blue_mat[]  = { 0.2, 0.2, 1.0, 1.0 };
  //  GLfloat white_mat[]  = { 1.0, 1.0, 1.0, 1.0 };
  //  GLfloat purple_mat[] = { 1.0, 0.2, 1.0, 1.0 };
  
  GLUquadricObj *qobj = gluNewQuadric();
  
  glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1, 1, -1, 1, -5.0, 5.0);//TODO this may be testcase dependent
  glMatrixMode(GL_MODELVIEW);
  
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  glPushMatrix();
  setCamera();
  
  
  // draw cells
  
  glLineWidth(4);
  glBegin(GL_LINES);
  for(int i = 0; i < numCells; ++i) {
    drawVelocityVector(centerCoordinates, velocity, temperature);
    centerCoordinates += 3;
    velocity += 3;
    temperature++;
  }
  glEnd();
  
  
  // draw particles
  
  drawParticles(__valid, __validIS, position, positionIS, density, densityIS,
                particleTemperature, particleTemperatureIS,
                runtime, ctx, qobj);
  
  
  glPopMatrix();
  
  
  /* This is very important!!!
   * Make sure buffered commands are finished!!!
   */
  glFinish();
  
  gluDeleteQuadric(qobj);
  
  
  
  int size = width * height * sizeof(GLfloat);
  *depthBuffer = (GLfloat*)calloc(1, size);
  glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, *depthBuffer);
  
}





static
void create_field_pointer(PhysicalRegion region,
                          FieldData* &field,
                          int fieldID,
                          ByteOffset stride[3],
                          Runtime* runtime,
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
        assert(strideVelocity[0].offset == 3 * sizeof(FieldData));
        break;
        
      case 1:
        create_field_pointer(*cell, centerCoordinates, cells_fields[field], strideCenter, runtime, ctx);
        assert(strideCenter[0].offset == 3 * sizeof(FieldData));
        break;
        
      case 2:
        create_field_pointer(*cell, temperature, cells_fields[field], strideTemperature, runtime, ctx);
        assert(strideTemperature[0].offset == sizeof(FieldData));
        break;
      default:
        std::cerr << "oops, field not found" << std::endl;
    }
  }
}



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
  outputFile.open(filePath.c_str());
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
  std::cout << "wrote " << counter << " cells to " << filePath << std::endl;
}



// TODO make this a template

static
void getBaseIndexSpaceBool(PhysicalRegion* particle, int fieldID, bool* &base, IndexSpace &indexSpace) {
  RegionAccessor<AccessorType::Generic, bool> acc = particle->get_field_accessor(fieldID).typeify<bool>();
  void* b = NULL;
  size_t stride = 0;
  acc.get_soa_parameters(b, stride);
  base = (bool*)b;
  assert(stride == sizeof(bool));
  indexSpace = particle->get_logical_region().get_index_space();
}


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
                        bool* &__valid,
                        IndexSpace &__validIS,
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
        
      case 6:
        getBaseIndexSpaceBool(particle, particles_fields[field], __valid, __validIS);
        break;
        
      default:
        std::cerr << "oops, field not found" << std::endl;
    }
  }
}




static
void writeParticlesToFile(std::string filePath,
                          bool* __validBase,
                          IndexSpace __validIS,
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
  outputFile.open(filePath.c_str());
  int counter = 0;
  
  IndexIterator __validIterator(runtime, ctx, __validIS);
  IndexIterator cellXIterator(runtime, ctx, cellXIS);
  IndexIterator cellYIterator(runtime, ctx, cellYIS);
  IndexIterator cellZIterator(runtime, ctx, cellZIS);
  IndexIterator positionIterator(runtime, ctx, positionIS);
  IndexIterator densityIterator(runtime, ctx, densityIS);
  IndexIterator particleTemperatureIterator(runtime, ctx, particleTemperatureIS);
  
  while(cellXIterator.has_next()) {
#define NEXT(FIELD) (FIELD##Base + FIELD##Iterator.next().value)
#define NEXT3(FIELD) (FIELD##Base + FIELD##Iterator.next().value * 3)
    bool valid = *NEXT(__valid);
    if(valid) {
      int cellX = *NEXT(cellX);
      int cellY = *NEXT(cellY);
      int cellZ = *NEXT(cellZ);
      FieldData* p = NEXT3(position);
      FieldData density = *NEXT(density);
      FieldData particleTemperature = *NEXT(particleTemperature);
      if(cellX > 0 && cellY > 0 && cellZ > 0) {
        outputFile << std::setprecision(10)
        << "(" << cellX << "," << cellY << "," << cellZ << ") "
        << p[0] << " " << p[1] << " " << p[2] << "  "
        << density << " "
        << particleTemperature
        << std::endl;
        counter++;
      }
    }
  }
  
  outputFile.close();
  std::cout << "wrote " << counter << " particles to " << filePath << std::endl;
}




static int volatile timeStep = 0;//to do pass this in
const int NUM_NODES = 4;//TODO eliminate this when timeStep is passed in

static
std::string fileName(std::string table, std::string ext, int timeStep, Rect<3> bounds) {
  char buffer[256];
  sprintf(buffer, "%s.%05d.%lld_%lld_%lld__%lld_%lld_%lld%s",
          table.c_str(),
          timeStep / NUM_NODES,//TODO don't divide by NUM_NODES once timeStep is passed in
          bounds.lo.x[0], bounds.lo.x[1], bounds.lo.x[2],
          bounds.hi.x[0], bounds.hi.x[1], bounds.hi.x[2],
          ext.c_str());
  return std::string(buffer);
}

static std::string dataFileName(std::string table, int timeStep, Rect<3> bounds) {
  return fileName(table, ".txt", timeStep, bounds);
}



static std::string imageFileName(std::string table, int timeStep, Rect<3> bounds) {
#ifdef IMAGE_FORMAT_TGA
  return fileName(table, ".tga", timeStep, bounds);
#else
  return fileName(table, ".ppm", timeStep, bounds);
#endif
}



#if 0
static void debugPrintVectors(std::string name, FieldData* c, Rect<3> bounds) {
  std::cout << "=== renderer " << name << " bounds " << bounds << " pointer " << c << " ===" << std::endl;
  for(unsigned i = 0; i < (unsigned)bounds.volume(); ++i) {
    std::cout << c << ")\t" << c[0] << " " << c[1] << " " << c[2] << std::endl;
    c += 3;
  }
}
#endif


// this is the entry point from regent

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
  
  accessCellData(cells, cells_fields, velocity, centerCoordinates, temperature,
                 strideCenter, strideVelocity, strideTemperature, bounds, runtime, ctx);
  
  if(writeFiles) {
    std::string cellsFileName = dataFileName("./out/cells", timeStep, bounds);
    writeCellsToFile(cellsFileName, bounds, velocity, centerCoordinates,
                     temperature, strideCenter, strideVelocity, strideTemperature);
  }
  
  int xLo = (int)bounds.lo.x[0];
  int yLo = (int)bounds.lo.x[1];
  int zLo = (int)bounds.lo.x[2];
  int xHi = (int)bounds.hi.x[0];
  int yHi = (int)bounds.hi.x[1];
  int zHi = (int)bounds.hi.x[2];
  numCells = (xHi - xLo) * (yHi - yLo) * (zHi - zLo);
  
  
  bool* __valid = NULL;
  IndexSpace __validIS;
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
  
  accessParticleData(particles, particles_fields, __valid, __validIS, cellX, cellXIS, cellY, cellYIS,
                     cellZ, cellZIS, position, positionIS, density, densityIS,
                     particleTemperature, particleTemperatureIS, runtime, ctx);
  
  if(writeFiles) {
    std::string particlesFileName = dataFileName("./out/particles", timeStep, bounds);
    writeParticlesToFile(particlesFileName, __valid, __validIS, cellX, cellXIS, cellY, cellYIS,
                         cellZ, cellZIS, position, positionIS, density, densityIS,
                         particleTemperature, particleTemperatureIS, runtime, ctx);
  }
  
  
  /* Create an RGBA-mode context */
#if OSMESA_MAJOR_VERSION * 100 + OSMESA_MINOR_VERSION >= 305
  /* specify Z, stencil, accum sizes */
  OSMesaContext mesaCtx = OSMesaCreateContextExt(OSMESA_RGBA, 32, 0, 0, NULL);
#else
  OSMesaContext mesaCtx = OSMesaCreateContext(OSMESA_RGBA, NULL);
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
               __valid, __validIS, position, positionIS, density, densityIS,
               particleTemperature, particleTemperatureIS,
               &rgbaBuffer, &depthBuffer, mesaCtx, runtime, ctx);
  
  write_ppm(imageFileName("./out/image", timeStep, bounds).c_str(), rgbaBuffer, width, height);
  
  std::string depthFileName = imageFileName("./out/depth", timeStep, bounds);
  FILE* depthFile = fopen(depthFileName.c_str(), "w");
  assert(depthFile != NULL);
  std::cout << "depthFile " << depthFile << depthFileName << std::endl;
  fprintf(depthFile, "%d %d\n", width, height);
  fwrite(depthBuffer, sizeof(GLfloat), width * height, depthFile);
  fclose(depthFile);
  std::cout << "wrote " << depthFileName << std::endl;
  
  /* free the image buffer */
  free(rgbaBuffer);
  free(depthBuffer);
  
  /* destroy the context */
  OSMesaDestroyContext(mesaCtx);
  
  timeStep++;
  
}
