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

//#define STANDALONE // debug

#define USE_SOFTWARE_OPENGL

#define _T {std::cout<<getpid()<<" "<<__FUNCTION__<<" "<<__FILE__<<":"<<__LINE__<<std::endl;}

#ifndef STANDALONE

#include "legion_c.h"
#include "legion_c_util.h"
#include "viz.h"

using namespace LegionRuntime::Arrays;
using namespace Legion;
using namespace LegionRuntime::Accessor;

#define NEXT(FIELD) (FIELD##Base + FIELD##Iterator.next().value)
#define NEXT3(FIELD) (FIELD##Base + FIELD##Iterator.next().value * 3)


#endif

const int width = 3840;
const int height = 2160;

#ifndef USE_SOFTWARE_RENDERING
// hardware accelerated OpenGL
#include <EGL/egl.h>

static const EGLint configAttribs[] = {
  EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
  EGL_BLUE_SIZE, 8,
  EGL_GREEN_SIZE, 8,
  EGL_RED_SIZE, 8,
  EGL_DEPTH_SIZE, 24,
  EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
  EGL_NONE
};

static const int pbufferWidth = width;
static const int pbufferHeight = height;

static const EGLint pbufferAttribs[] = {
  EGL_WIDTH, pbufferWidth,
  EGL_HEIGHT, pbufferHeight,
  EGL_NONE,
};

#endif

#include <cfloat>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <iomanip>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <malloc.h>


// search for the word "testcase" to find all of the testcase dependent changes
// change this for each testcase to give the total num particles
#define EXPECTED_NUM_PARTICLES 1000000 // this is for the 512x512x256 taylor testcase
#define NUM_NODES 4 // only valid for sapling or four node piz daint!
#define EXPECTED_PARTICLES_PER_NODE (EXPECTED_NUM_PARTICLES / NUM_NODES)

const int numVisibleParticlesPerNode = 256;


static bool writeFiles(int timeStep) {
  //return true;
  return false;
  //return timeStep % 100 == 0;
}


#ifdef STANDALONE

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include "OpenGL/glu.h"
#else
#include "GL/osmesa.h"
#include "GL/glu.h"
#endif

typedef double FieldData;

FieldData* centerCoordinates;
FieldData* velocity;
FieldData* temperature;
int totalCells;
int numCells[3];
FieldData min[3];
FieldData max[3];


#endif



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
    printf("successfully wrote %s\n", filename);
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
  const GLfloat min = 0.0;
  const GLfloat max = 40.0f;
  const GLfloat Kmin = 0.0f;
  const GLfloat Kmax = 10000.0f;
  GLfloat scaledTemperature = (temperature - min) * ((Kmax - Kmin) / (max - min));
  return temperatureToColor(scaledTemperature, color);
}


static void drawVelocityVector(FieldData* centerCoordinate,
                               FieldData* velocity,
                               FieldData* temperature) {
  GLfloat scale = 0.08;
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



static void drawParticle(GLUquadricObj* qobj, float* position, float density, float particleTemperature) {
  
  GLfloat t = particleTemperature;
  GLfloat color[4];
  scaledTemperatureToColor(t, color);
  glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, color);
  glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color);
  glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
  const GLfloat verticalOffset = 0.0f;//TODO this is testcase dependent
  
  glPushMatrix();
  glTranslatef(position[0], position[1], position[2] + verticalOffset);
  const GLfloat densityScale = 5.0e-6;
  gluSphere(qobj, density * densityScale, 7, 7);
  glPopMatrix();
}


#ifndef STANDALONE


static void drawParticles(bool* __valid,
                          FieldData* position,
                          FieldData* density,
                          FieldData* particleTemperature,
                          bool* tracking,
                          ByteOffset __validStride[1],
                          ByteOffset positionStride[1],
                          ByteOffset densityStride[1],
                          ByteOffset particleTemperatureStride[1],
                          ByteOffset trackingStride[1],
                          GLUquadricObj* qobj,
                          Runtime* runtime) {
  
  FieldData minCenter[3] = { FLT_MAX };
  FieldData maxCenter[3] = { 0 };
  
  int numTracking = 0;
  int numParticles = 0;
  int numDrawn = 0;
  
  const long RAND_MAX_ = (long)(powf(2.0f, 31.0f) - 1.0f);
  const long randomThreshold = RAND_MAX_ * numVisibleParticlesPerNode / EXPECTED_PARTICLES_PER_NODE;
  srandom(0);//same every frame

  for(unsigned particle = 0; particle < EXPECTED_PARTICLES_PER_NODE; ++particle) {
    __valid += __validStride[0].offset / sizeof(*__valid);
    FieldData* p = position;
    position += positionStride[0].offset / sizeof(*position);
    float pos[3] = { (float)p[0], (float)p[1], (float)p[2] };
    float d = *density;
    density += densityStride[0].offset / sizeof(*density);
    float particleTemp = *particleTemperature;
    particleTemperature += particleTemperatureStride[0].offset / sizeof(*particleTemperature);
    tracking += trackingStride[0].offset / sizeof(*tracking);
    if(random() <= randomThreshold) {
      drawParticle(qobj, pos, d, particleTemp);
      numDrawn++;
      numTracking++;
    }
    for(unsigned i = 0; i < 3; ++i) {
      if(pos[i] > maxCenter[i]) {
        maxCenter[i] = pos[i];
      }
      if(pos[i] < minCenter[i]) {
        minCenter[i] = pos[i];
      }
    }
    numParticles++;
  }
  
  //std::cout << std::endl;
  //std::cout << getpid() << " " << "particles " << numParticles << " tracking " << numTracking << " drawn " << numDrawn << std::endl;
  //std::cout << getpid() << " " << "particle position min " << minCenter[0] << "," << minCenter[1] << "," << minCenter[2];
  //std::cout << getpid() << " " << " max " << maxCenter[0] << "," << maxCenter[1] << "," << maxCenter[2] << std::endl;
}




#else



static void drawParticles(std::string particleFilePath,
                          GLUquadricObj* qobj) {
  
  std::ifstream particleFile;
  particleFile.open(particleFilePath.c_str(), std::ifstream::in);
  int numDrawn = 0;
  int numParticles = 0;
  int numRandom = 0;
  int numTracking = 0;
  FieldData minCenter[3] = { FLT_MAX };
  FieldData maxCenter[3] = { 0 };
  
  char buffer[256];
  
  srandom(0);
  const long RAND_MAX_ = (long)(powf(2.0f, 31.0f) - 1.0f);
  const long randomThreshold = RAND_MAX_ * numVisibleParticlesPerNode / EXPECTED_PARTICLES_PER_NODE;
  
  while(particleFile.getline(buffer, sizeof(buffer))) {
    int cellX, cellY, cellZ;
    FieldData position[3];
    FieldData density;
    FieldData particleTemperature;
    sscanf(buffer, "(%d,%d,%d) %lf %lf %lf %lf %lf",
           &cellX, &cellY, &cellZ,
           position, position + 1, position + 2,
           &density,
           &particleTemperature);
    
    bool tracking = (numTracking < numVisibleParticlesPerNode);
    if(tracking) numTracking++;
    bool randomlySelected = random() < randomThreshold;
    if(randomlySelected) numRandom++;
    if(tracking || randomlySelected) {
      float pos[3] = { (float)position[0], (float)position[1], (float)position[2] };
      drawParticle(qobj, pos, density, particleTemperature);
      numDrawn++;
    }
    for(unsigned i = 0; i < 3; ++i) {
      if(position[i] > maxCenter[i]) {
        maxCenter[i] = position[i];
      }
      if(position[i] < minCenter[i]) {
        minCenter[i] = position[i];
      }
    }
    
    numParticles++;
  }
  particleFile.close();
  std::cout << "particles " << numParticles << " tracking " << numTracking << " random " << numRandom << " drawn " << numDrawn << std::endl;
  std::cout << "particle position min " << minCenter[0] << "," << minCenter[1] << "," << minCenter[2];
  std::cout << " max " << maxCenter[0] << "," << maxCenter[1] << "," << maxCenter[2] << std::endl;
  
}

#endif


/* 512x512x256 taylor
 cell min 1611.64,807.389,807.389
 cell max 2409.6,1605.35,1605.35
 reading particle data from piz_daint_output/particles.00000.256_128_128__383_255_255.txt
 calling cxx_render with output to foo.ppm
 mean velocity magnitude 2.57586e-13
 */

static void setCamera() {
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  // change this for each testcase to hold the data
  glOrtho(-6, 6, -6, 6, -20, 20);
  //gluPerspective(120, width / height, 0, 4000);
  
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  // possbly change the at point for each testcase
  gluLookAt(/*eye*/-1, -0.5, 4.00, /*at*/3, 3.45, 3, /*up*/0.0, 0.0, 1.0);
}



static void drawAveragedCells(FieldData* centerCoordinates, FieldData* velocity, FieldData* temperature, int numCells[3], GLUquadricObj *qobj) {
  // set the parameters for averaging here
  const unsigned zFactor = 32;//average over xFactor*yFactor*zFactor cells
  const unsigned xFactor = 32;
  const unsigned yFactor = 32;
  const unsigned numMeanCells[] = {numCells[0] / xFactor, numCells[1] / yFactor, numCells[2] / zFactor };
  
  FieldData minCenter[3] = { FLT_MAX };
  FieldData maxCenter[3] = { 0 };
  FieldData minMean[3] = { FLT_MAX };
  FieldData maxMean[3] = { 0 };
  
  const unsigned totalMeanCells = numMeanCells[0] * numMeanCells[1] * numMeanCells[2];
  float totalVelocityMagnitude = 0;
  
  glBegin(GL_LINES);
  
  for(unsigned meanZ = 0; meanZ < numMeanCells[2]; ++meanZ) {
    for(unsigned meanY = 0; meanY < numMeanCells[1]; ++meanY) {
      for(unsigned meanX = 0; meanX < numMeanCells[0]; ++meanX) {
        
        const unsigned baseCell[3] = { meanX * xFactor, meanY * yFactor, meanZ * zFactor };
        FieldData meanCenterCoordinates[3] = { 0 };
        FieldData meanVelocity[3] = { 0 };
        FieldData meanTemperature = 0;
        
        for(unsigned dz = 0; dz < zFactor; ++dz) {
          for(unsigned dy = 0; dy < yFactor; ++dy) {
            for(unsigned dx = 0; dx < xFactor; ++dx) {
              int cellX = baseCell[0] + dx;
              int cellY = baseCell[1] + dy;
              int cellZ = baseCell[2] + dz;
              int cellIndex = cellX + (cellY * numCells[0]) + (cellZ * numCells[0] * numCells[1]);
              FieldData* cellCenterCoordinates = centerCoordinates + 3 * cellIndex;
              FieldData* cellVelocity = velocity + 3 * cellIndex;
              FieldData* cellTemperature = temperature + cellIndex;
              
              for(unsigned i = 0; i < 3; ++i) {
                meanCenterCoordinates[i] += cellCenterCoordinates[i];
                meanVelocity[i] += cellVelocity[i];
                if(cellCenterCoordinates[i] > maxCenter[i]) {
                  maxCenter[i] = cellCenterCoordinates[i];
                }
                if(cellCenterCoordinates[i] < minCenter[i]) {
                  minCenter[i] = cellCenterCoordinates[i];
                }
              }
              meanTemperature += cellTemperature[0];
            }
          }
        }
        
        for(unsigned i = 0; i < 3; ++i) {
          meanCenterCoordinates[i] /= (xFactor * yFactor * zFactor);
          meanVelocity[i] /= (xFactor * yFactor * zFactor);
        }
        meanTemperature /= (xFactor * yFactor * zFactor);
        for(unsigned i = 0; i < 3; ++i) {
          if(meanCenterCoordinates[i] > maxMean[i]) {
            maxMean[i] = meanCenterCoordinates[i];
          }
          if(meanCenterCoordinates[i] < minMean[i]) {
            minMean[i] = meanCenterCoordinates[i];
          }
        }
        
        drawVelocityVector(meanCenterCoordinates, meanVelocity, &meanTemperature);
        float magnitudeSquared = meanVelocity[0] * meanVelocity[0] + meanVelocity[1] * meanVelocity[1] + meanVelocity[2] * meanVelocity[2];
        float velocityMagnitude = sqrtf(magnitudeSquared);
        totalVelocityMagnitude += velocityMagnitude;
      }
    }
  }
  
  glEnd();
  
  totalVelocityMagnitude /= totalMeanCells;
  
  std::cout << "mean velocity magnitude " << totalVelocityMagnitude;
  std::cout << " cell center min " << minCenter[0] << "," << minCenter[1] << "," << minCenter[2]
  << " max " << maxCenter[0] << "," << maxCenter[1] << "," << maxCenter[2] << std::endl;
  std::cout << " cell mean center min " << minMean[0] << "," << minMean[1] << "," << minMean[2]
  << " max mean " << maxMean[0] << "," << maxMean[1] << "," << maxMean[2] << std::endl;
  
}




#ifdef STANDALONE

void render_image(int width,
                  int height,
                  FieldData* centerCoordinates,
                  FieldData* velocity,
                  FieldData* temperature,
                  int totalCells,
                  std::string particleFilePath,
                  GLfloat** rgbaBuffer,
                  GLfloat** depthBuffer,
                  int numCells[3])


#else



void render_image(int width,
                  int height,
                  FieldData* centerCoordinates,
                  FieldData* velocity,
                  FieldData* temperature,
                  int totalCells,
                  bool* __valid,
                  FieldData* position,
                  FieldData* density,
                  FieldData* particleTemperature,
                  bool* tracking,
                  ByteOffset __validStride[1],
                  ByteOffset positionStride[1],
                  ByteOffset densityStride[1],
                  ByteOffset particleTemperatureStride[1],
                  ByteOffset trackingStride[1],
                  GLfloat** rgbaBuffer,
                  GLfloat** depthBuffer,
                  Runtime* runtime,
                  int numCells[3])
#endif
{
  
  
  //GLfloat light_ambient[] = { 0.5, 0.5, 0.5, 1.0 };
  GLfloat light_diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat light_specular[] = { 1.0, 1.0, 1.0, 1.0 };
  GLfloat light_position[] = { -15.0, 15.0, 50.0, 1.0 };
  
  //  GLfloat red_mat[]   = { 1.0, 0.2, 0.2, 1.0 };
  //  GLfloat green_mat[] = { 0.2, 1.0, 0.2, 0.5 };
  //  GLfloat blue_mat[]  = { 0.2, 0.2, 1.0, 1.0 };
  //  GLfloat white_mat[]  = { 1.0, 1.0, 1.0, 1.0 };
  //  GLfloat purple_mat[] = { 1.0, 0.2, 1.0, 1.0 };
  
  GLUquadricObj *qobj = gluNewQuadric();
  
//  glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
  glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
  glLightfv(GL_LIGHT0, GL_POSITION, light_position);
  glClearColor((GLclampf)0.8, (GLclampf)0.9, (GLclampf)0.95, (GLclampf)1.0);
  
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_DEPTH_TEST);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  
  setCamera();
  
  
  // draw cells
  
  float totalVelocityMagnitude = 0;
  glLineWidth(6);
  
  bool is1D = (numCells[1] == 1 && numCells[2] == 1);
  if(is1D) {
    
    glBegin(GL_LINES);
    for(int i = 0; i < totalCells; ++i) {
      drawVelocityVector(centerCoordinates, velocity, temperature);
      float magnitudeSquared = velocity[0] * velocity[0] + velocity[1] * velocity[1] + velocity[2] * velocity[2];
      float velocityMagnitude = sqrtf(magnitudeSquared);
      totalVelocityMagnitude += velocityMagnitude;
      centerCoordinates += 3;
      velocity += 3;
      temperature++;
    }
    glEnd();
    
    float meanVelocityMagnitude = totalVelocityMagnitude / totalCells;
    std::cout << "mean velocity magnitude " << meanVelocityMagnitude << std::endl;
    
  } else {
    
    drawAveragedCells(centerCoordinates, velocity, temperature, numCells, qobj);
    
  }
  
  
  // draw particles
  
#ifndef STANDALONE
  
  
  

  drawParticles(__valid, position, density, particleTemperature, tracking,
                __validStride, positionStride, densityStride, particleTemperatureStride, trackingStride,
                qobj, runtime);
  
#else
  
  drawParticles(particleFilePath, qobj);
  
#endif
  
  glPopMatrix();
  
  /* This is very important!!!
   * Make sure buffered commands are finished!!!
   */
  glFinish();
  
  gluDeleteQuadric(qobj);
  
#ifndef USE_SOFTWARE_RENDERING
  int rgbaSize = width * height * 4 * sizeof(GLfloat);
  *rgbaBuffer = (GLfloat*)calloc(1, rgbaSize);
  glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, *rgbaBuffer);
#endif
  
  int depthSize = width * height * sizeof(GLfloat);
  *depthBuffer = (GLfloat*)calloc(1, depthSize);
  glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, *depthBuffer);
  
}



#ifndef STANDALONE

static
void create_field_pointer(PhysicalRegion region,
                          FieldData* &field,
                          int fieldID,
                          ByteOffset stride[3],
                          Runtime* runtime) {
  
  Domain indexSpaceDomain = runtime->get_index_space_domain(region.get_logical_region().get_index_space());
  Rect<3> bounds = indexSpaceDomain.get_rect<3>();
  RegionAccessor<AccessorType::Generic, FieldData> acc = region.get_field_accessor(fieldID).typeify<FieldData>();
  Rect<3> tempBounds;
  field = acc.raw_rect_ptr<3>(bounds, tempBounds, stride);
  assert(bounds == tempBounds);
}


static
void create_field_pointer(PhysicalRegion region,
                          float* &field,
                          int fieldID,
                          ByteOffset stride[3],
                          Runtime* runtime) {
  
  Domain indexSpaceDomain = runtime->get_index_space_domain(region.get_logical_region().get_index_space());
  Rect<3> bounds = indexSpaceDomain.get_rect<3>();
  RegionAccessor<AccessorType::Generic, float> acc = region.get_field_accessor(fieldID).typeify<float>();
  Rect<3> tempBounds;
  field = acc.raw_rect_ptr<3>(bounds, tempBounds, stride);
  assert(bounds == tempBounds);
}


static
void create_field_pointer1(PhysicalRegion region,
                           FieldData* &field,
                           int fieldID,
                           ByteOffset stride[1],
                           Runtime* runtime) {
  
  Domain indexSpaceDomain = runtime->get_index_space_domain(region.get_logical_region().get_index_space());
  Rect<1> bounds = indexSpaceDomain.get_rect<1>();
  RegionAccessor<AccessorType::Generic, FieldData> acc = region.get_field_accessor(fieldID).typeify<FieldData>();
  Rect<1> tempBounds;
  field = acc.raw_rect_ptr<1>(bounds, tempBounds, stride);
  assert(bounds == tempBounds);
}

#if 0
static
void create_field_pointer1(PhysicalRegion region,
                           float* &field,
                           int fieldID,
                           ByteOffset stride[1],
                           Runtime* runtime) {
  
  Domain indexSpaceDomain = runtime->get_index_space_domain(region.get_logical_region().get_index_space());
  Rect<1> bounds = indexSpaceDomain.get_rect<1>();
  RegionAccessor<AccessorType::Generic, float> acc = region.get_field_accessor(fieldID).typeify<float>();
  Rect<1> tempBounds;
  field = acc.raw_rect_ptr<1>(bounds, tempBounds, stride);
  assert(bounds == tempBounds);
}
#endif


static
void create_field_pointer1(PhysicalRegion region,
                           int* &field,
                           int fieldID,
                           ByteOffset stride[1],
                           Runtime* runtime) {
  
  Domain indexSpaceDomain = runtime->get_index_space_domain(region.get_logical_region().get_index_space());
  Rect<1> bounds = indexSpaceDomain.get_rect<1>();
  RegionAccessor<AccessorType::Generic, int> acc = region.get_field_accessor(fieldID).typeify<int>();
  Rect<1> tempBounds;
  field = acc.raw_rect_ptr<1>(bounds, tempBounds, stride);
  assert(bounds == tempBounds);
}



static
void create_field_pointer1(PhysicalRegion region,
                           bool* &field,
                           int fieldID,
                           ByteOffset stride[1],
                           Runtime* runtime) {
  
  Domain indexSpaceDomain = runtime->get_index_space_domain(region.get_logical_region().get_index_space());
  Rect<1> bounds = indexSpaceDomain.get_rect<1>();
  RegionAccessor<AccessorType::Generic, bool> acc = region.get_field_accessor(fieldID).typeify<bool>();
  Rect<1> tempBounds;
  field = acc.raw_rect_ptr<1>(bounds, tempBounds, stride);
  assert(bounds == tempBounds);
}






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
                    Runtime* runtime) {
  
  PhysicalRegion* cell = CObjectWrapper::unwrap(cells[0]);
  std::vector<legion_field_id_t> fields;
  cell->get_fields(fields);
  
  for(unsigned field = 0; field < fields.size(); ++field) {
    PhysicalRegion* cell = CObjectWrapper::unwrap(cells[field]);
    Domain indexSpaceDomain = runtime->get_index_space_domain(cell->get_logical_region().get_index_space());
    bounds = indexSpaceDomain.get_rect<3>();
    
    switch(field) {
      case 0:
        create_field_pointer(*cell, velocity, cells_fields[field], strideVelocity, runtime);
        assert(strideVelocity[0].offset == 3 * sizeof(FieldData));
        break;
        
      case 1:
        create_field_pointer(*cell, centerCoordinates, cells_fields[field], strideCenter, runtime);
        assert(strideCenter[0].offset == 3 * sizeof(FieldData));
        break;
        
      case 2:
        create_field_pointer(*cell, temperature, cells_fields[field], strideTemperature, runtime);
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
                      FieldData* velocity,
                      FieldData* centerCoordinates,
                      FieldData* temperature,
                      ByteOffset strideCenter[3],
                      ByteOffset strideVelocity[3],
                      ByteOffset strideTemperature[3]) {
  
  std::ofstream outputFile;
  outputFile.open(filePath.c_str());
  outputFile << bounds << std::endl;
  int counter = 0;
  
  for(coord_t z = bounds.lo.x[2]; z <= bounds.hi.x[2]; ++z) {
    for(coord_t y = bounds.lo.x[1]; y <= bounds.hi.x[1]; ++y) {
      for(coord_t x = bounds.lo.x[0]; x <= bounds.hi.x[0]; ++x) {
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


#if 0
// TODO make this a template

static
void getBaseIndexSpaceBool(PhysicalRegion* particle, int fieldID, bool* &base, IndexSpace &indexSpace) {
  RegionAccessor<AccessorType::Generic, bool> acc = particle->get_field_accessor(fieldID).typeify<bool>();
  void* b = NULL;
  size_t stride = stride == sizeof(bool);
  acc.get_soa_parameters(b, stride);
  base = (bool*)b;
  assert(stride == sizeof(bool));
  indexSpace = particle->get_logical_region().get_index_space();
}


static
void getBaseIndexSpaceInt(PhysicalRegion* particle, int fieldID, int* &base, IndexSpace &indexSpace) {
  RegionAccessor<AccessorType::Generic, int> acc = particle->get_field_accessor(fieldID).typeify<int>();
  void* b = NULL;
  size_t stride = sizeof(int);
  acc.get_soa_parameters(b, stride);
  base = (int*)b;
  assert(stride == sizeof(int));
  indexSpace = particle->get_logical_region().get_index_space();
}


static
void getBaseIndexSpaceFieldData_impl(PhysicalRegion* particle, int fieldID, FieldData* &base, IndexSpace &indexSpace, int numFields) {
  RegionAccessor<AccessorType::Generic, FieldData> acc = particle->get_field_accessor(fieldID).typeify<FieldData>();
  void* b = NULL;
  size_t stride = sizeof(FieldData) * numFields;
  acc.get_soa_parameters(b, stride);
  base = (FieldData*)b;
  assert(stride == sizeof(FieldData) * numFields);
  indexSpace = particle->get_logical_region().get_index_space();
}


static
void getBaseIndexSpaceFieldData(PhysicalRegion* particle, int fieldID, FieldData* &base, IndexSpace &indexSpace) {
  getBaseIndexSpaceFieldData_impl(particle, fieldID, base, indexSpace, 1);
}


static
void getBaseIndexSpaceFieldData3(PhysicalRegion* particle, int fieldID, FieldData* &base, IndexSpace &indexSpace) {
  getBaseIndexSpaceFieldData_impl(particle, fieldID, base, indexSpace, 3);
}
#endif


static
void accessParticleData(legion_physical_region_t *particles,
                        legion_field_id_t *particles_fields,
                        bool* &__valid,
                        ByteOffset __validStride[1],
                        int* &cellX,
                        ByteOffset cellXStride[1],
                        int* &cellY,
                        ByteOffset cellYStride[1],
                        int* &cellZ,
                        ByteOffset cellZStride[1],
                        FieldData* &position,
                        ByteOffset positionStride[1],
                        FieldData* &density,
                        ByteOffset densityStride[1],
                        FieldData* &particleTemperature,
                        ByteOffset particleTemperatureStride[1],
                        bool* &tracking,
                        ByteOffset trackingStride[1],
                        Runtime* runtime) {
  
  PhysicalRegion* particle = CObjectWrapper::unwrap(particles[0]);
  std::vector<legion_field_id_t> fields;
  particle->get_fields(fields);
  
  for(unsigned field = 0; field < fields.size(); ++field) {
    PhysicalRegion* particle = CObjectWrapper::unwrap(particles[field]);
    
    switch(field) {
      case 0:
        create_field_pointer1(*particle, cellX, particles_fields[field], cellXStride, runtime);
        assert(cellXStride[0].offset == sizeof(int));
        break;
        
      case 1:
        create_field_pointer1(*particle, cellY, particles_fields[field], cellYStride, runtime);
        assert(cellYStride[0].offset == sizeof(int));
        break;
        
      case 2:
        create_field_pointer1(*particle, cellZ, particles_fields[field], cellZStride, runtime);
        assert(cellZStride[0].offset == sizeof(int));
        break;
        
      case 3:
        create_field_pointer1(*particle, position, particles_fields[field], positionStride, runtime);
        assert(positionStride[0].offset == 3 * sizeof(FieldData));
        break;
        
      case 4:
        create_field_pointer1(*particle, density, particles_fields[field], densityStride, runtime);
        assert(densityStride[0].offset == sizeof(FieldData));
        break;
        
      case 5:
        create_field_pointer1(*particle, particleTemperature, particles_fields[field], particleTemperatureStride, runtime);
        assert(particleTemperatureStride[0].offset == sizeof(FieldData));
        break;
        
      case 6:
        create_field_pointer1(*particle, tracking, particles_fields[field], trackingStride, runtime);
        assert(trackingStride[0].offset == sizeof(bool));
        break;
        
      case 7:
        create_field_pointer1(*particle, __valid, particles_fields[field], __validStride, runtime);
        assert(__validStride[0].offset == sizeof(bool));
        break;
        
      default:
        std::cerr << "oops, field not found" << std::endl;
    }
  }
}



static
void writeParticlesToFile(std::string filePath,
                          bool* __valid,
                          int* cellX,
                          int* cellY,
                          int* cellZ,
                          FieldData* position,
                          FieldData* density,
                          FieldData* particleTemperature,
                          bool* tracking,
                          ByteOffset __validStride[1],
                          ByteOffset cellXStride[1],
                          ByteOffset cellYStride[1],
                          ByteOffset cellZStride[1],
                          ByteOffset positionStride[1],
                          ByteOffset densityStride[1],
                          ByteOffset particleTemperatureStride[1],
                          ByteOffset trackingStride[1],
                          Runtime* runtime) {
  
  std::ofstream outputFile;
  outputFile.open(filePath.c_str());
  int counter = 0;
  int numParticles = 0;
  
  while(numParticles < EXPECTED_PARTICLES_PER_NODE) {
    bool valid = *__valid;
    __valid += __validStride[0].offset / sizeof(*__valid);
    int cX = *cellX;
    cellX += cellXStride[0].offset / sizeof(*cellX);
    int cY = *cellY;
    cellY += cellYStride[0].offset / sizeof(*cellY);
    int cZ = *cellZ;
    cellZ += cellZStride[0].offset / sizeof(*cellZ);
    FieldData* p = position;
    position += positionStride[0].offset / sizeof(*position);
    FieldData d = *density;
    density += densityStride[0].offset / sizeof(*density);
    FieldData pt = *particleTemperature;
    particleTemperature += particleTemperatureStride[0].offset / sizeof(*particleTemperature);
    bool t = *tracking;
    tracking += trackingStride[0].offset / sizeof(*tracking);
    if(valid) {
      outputFile << std::setprecision(10)
      << "(" << cX << "," << cY << "," << cZ << ") "
      << p[0] << " " << p[1] << " " << p[2] << "  "
      << d << " "
      << pt << " "
      << t
      << std::endl;
      counter++;
    }
    numParticles++;
  }
  
  outputFile.close();
  std::cout << "wrote " << counter << " particles to " << filePath << std::endl;
}




static
std::string fileName(std::string table, std::string ext, int timeStep) {
  char buffer[256];
  sprintf(buffer, "%s.%05d%s",
          table.c_str(),
          timeStep,
          ext.c_str());
  return std::string(buffer);
}

static
std::string fileName(std::string table, std::string ext, int timeStep, Rect<3> bounds) {
  char buffer[256];
  sprintf(buffer, "%s.%05d.%lld_%lld_%lld__%lld_%lld_%lld%s",
          table.c_str(),
          timeStep,
          bounds.lo.x[0], bounds.lo.x[1], bounds.lo.x[2],
          bounds.hi.x[0], bounds.hi.x[1], bounds.hi.x[2],
          ext.c_str());
  return std::string(buffer);
}


static std::string dataFileName(std::string table, int timeStep, Rect<3> bounds) {
  return fileName(table, ".txt", timeStep, bounds);
}



static std::string imageFileName(std::string table, std::string ext, int timeStep, Rect<3> bounds) {
  return fileName(table, ext, timeStep, bounds);
}

static std::string imageFileName(std::string table, std::string ext, int timeStep) {
  return fileName(table, ext, timeStep);
}



static void writeRenderedPixelsToImageFragment(GLfloat* rgba,
                                               GLfloat* depth,
                                               Runtime* runtime,
                                               legion_physical_region_t* imageFragment,
                                               legion_field_id_t* imageFragment_fields,
                                               std::vector<legion_field_id_t> fields,
                                               Rect<3> bounds) {
  
  float* R = NULL;
  float* G = NULL;
  float* B = NULL;
  float* A = NULL;
  float* Z = NULL;
  float* UserData = NULL;
  
  ByteOffset strideR[3];
  ByteOffset strideG[3];
  ByteOffset strideB[3];
  ByteOffset strideA[3];
  ByteOffset strideZ[3];
  ByteOffset strideUserData[3];
  
  for(unsigned field = 0; field < fields.size(); ++field) {
    PhysicalRegion* image = CObjectWrapper::unwrap(imageFragment[field]);
    switch(field) {
      case 0:
        create_field_pointer(*image, R, imageFragment_fields[field], strideR, runtime);
        break;
      case 1:
        create_field_pointer(*image, G, imageFragment_fields[field], strideG, runtime);
        break;
      case 2:
        create_field_pointer(*image, B, imageFragment_fields[field], strideB, runtime);
        break;
      case 3:
        create_field_pointer(*image, A, imageFragment_fields[field], strideA, runtime);
        break;
      case 4:
        create_field_pointer(*image, Z, imageFragment_fields[field], strideZ, runtime);
        break;
      case 5:
        create_field_pointer(*image, UserData, imageFragment_fields[field], strideUserData, runtime);
        break;
    }
  }
  
  unsigned pixelCounter = 0;
  
  for(coord_t y = bounds.lo.x[1]; y <= bounds.hi.x[1]; ++y) {
    for(coord_t x = bounds.lo.x[0]; x <= bounds.hi.x[0]; ++x) {
      *R = rgba[0];
      *G = rgba[1];
      *B = rgba[2];
      *A = rgba[3];
      rgba += 4;
      *Z = depth[0];
      depth++;
      *UserData = 0.0;
      R += strideR[0].offset / sizeof(*R);
      G += strideG[0].offset / sizeof(*G);
      B += strideB[0].offset / sizeof(*B);
      A += strideA[0].offset / sizeof(*A);
      Z += strideZ[0].offset / sizeof(*Z);
      UserData += strideUserData[0].offset / sizeof(*UserData);
      pixelCounter++;
    }
  }
}



static void
writeRenderedPixelsToImageFragments(GLfloat* rgbaBuffer,
                                    GLfloat* depthBuffer,
                                    Runtime* runtime,
                                    legion_physical_region_t* imageFragment[],
                                    legion_field_id_t* imageFragment_fields[],
                                    int width,
                                    int height) {
  
  PhysicalRegion* fragment = CObjectWrapper::unwrap(imageFragment[0][0]);
  std::vector<legion_field_id_t> fields;
  fragment->get_fields(fields);
  const int expectedNumFields = 6;
  assert(fields.size() == expectedNumFields);
  Domain indexSpaceDomain = runtime->get_index_space_domain(fragment->get_logical_region().get_index_space());
  Rect<3> bounds = indexSpaceDomain.get_rect<3>();
  int fragmentHeight = (int)(bounds.hi.x[1] - bounds.lo.x[1]) + 1;
  assert(fragmentHeight > 0);
  int numFragmentsPerImage = height / fragmentHeight;
  
  for(int i = 0; i < numFragmentsPerImage; ++i) {
    GLfloat* rgba = rgbaBuffer + i * fragmentHeight * width * 4;
    GLfloat* depth = depthBuffer + i * fragmentHeight * width;
    writeRenderedPixelsToImageFragment(rgba, depth, runtime, imageFragment[i], imageFragment_fields[i], fields, bounds);
  }
}



#endif


#ifdef USE_SOFTWARE_RENDERING



static void createGraphicsContext(OSMesaContext &mesaCtx,
                                  GLfloat* &rgbaBuffer) {
  
  // non-accelerated OpenGL
  /* Create an RGBA-mode context */
#if OSMESA_MAJOR_VERSION * 100 + OSMESA_MINOR_VERSION >= 305
  /* specify Z, stencil, accum sizes */
  mesaCtx = OSMesaCreateContextExt(GL_RGBA, 32, 0, 0, NULL);
#else
  mesaCtx = OSMesaCreateContext(GL_RGBA, NULL);
#endif
  if (!mesaCtx) {
    printf("OSMesaCreateContext failed!\n");
    return;
  }
  /* Allocate the image buffer */
  const int fieldsPerPixel = 4;
  rgbaBuffer = (GLfloat *) malloc(width * height * fieldsPerPixel * sizeof(GLfloat));
  if (!rgbaBuffer) {
    printf("Alloc image buffer failed!\n");
    return;
  }

  /* Bind the buffer to the context and make it current */
  if (!OSMesaMakeCurrent(mesaCtx, rgbaBuffer, GL_FLOAT, width, height)) {
    printf("OSMesaMakeCurrent failed!\n");
    return;
  }
}

static void destroyGraphicsContext(OSMesaContext mesaCtx) {
  /* destroy the context */
  OSMesaDestroyContext(mesaCtx);
}


#else



static void createGraphicsContext(EGLContext &eglCtx, EGLDisplay &eglDpy) {
  
  // hardware accelerated OpenGL
  // https://devblogs.nvidia.com/parallelforall/egl-eye-opengl-visualization-without-x-server/
  eglDpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  EGLint major, minor;
  eglInitialize(eglDpy, &major, &minor);
  
  // 2. Select an appropriate configuration
  EGLint numConfigs;
  EGLConfig eglCfg;
  eglChooseConfig(eglDpy, configAttribs, &eglCfg, 1, &numConfigs);
  
  // 3. Create a surface
  EGLSurface eglSurf = eglCreatePbufferSurface(eglDpy, eglCfg,
                                               pbufferAttribs);
  
  // 4. Bind the API
  eglBindAPI(EGL_OPENGL_API);
  
  // 5. Create a context and make it current
  eglCtx = eglCreateContext(eglDpy, eglCfg, EGL_NO_CONTEXT,
                                       NULL);
  eglMakeCurrent(eglDpy, eglSurf, eglSurf, eglCtx);
  
}

static void destroyGraphicsContext(EGLDisplay eglDpy) {
  eglTerminate(eglDpy);

}


#endif



#ifdef STANDALONE

void cxx_render(std::string particleFilePath, std::string outputFileName, int numCells[3])

#else

void cxx_render(legion_runtime_t runtime_,
                legion_physical_region_t *cells,
                legion_field_id_t *cells_fields,
                legion_physical_region_t *particles,
                legion_field_id_t *particles_fields,
                // CODEGEN: legion_physical_region_t_imageFragmentXComma
                int xnum,
                int ynum,
                int znum,
                int timeStepNumber)
#endif
{
  
  
#ifndef STANDALONE
  
  // CODEGEN: legion_physical_region_t_imageFragment_arrays
  
  
  Runtime *runtime = CObjectWrapper::unwrap(runtime_);
  
  FieldData* centerCoordinates = NULL;
  FieldData* velocity = NULL;
  FieldData* temperature = NULL;
  ByteOffset strideCenter[3];
  ByteOffset strideVelocity[3];
  ByteOffset strideTemperature[3];
  Rect<3> bounds = Rect<3>(Point<3>::ZEROES(), Point<3>::ZEROES());
  int totalCells = 0;
  
  accessCellData(cells, cells_fields, velocity, centerCoordinates, temperature,
                 strideCenter, strideVelocity, strideTemperature, bounds, runtime);
  
  if(writeFiles(timeStepNumber)) {
    std::string cellsFileName = dataFileName("./out/cells", timeStepNumber, bounds);
    writeCellsToFile(cellsFileName, bounds, velocity, centerCoordinates,
                     temperature, strideCenter, strideVelocity, strideTemperature);
  }
  
  int xLo = (int)bounds.lo.x[0];
  int yLo = (int)bounds.lo.x[1];
  int zLo = (int)bounds.lo.x[2];
  int xHi = (int)bounds.hi.x[0];
  int yHi = (int)bounds.hi.x[1];
  int zHi = (int)bounds.hi.x[2];
  int numCells[3] = { (xHi - xLo + 1), (yHi - yLo + 1), (zHi - zLo + 1) };
  totalCells = numCells[0] * numCells[1] * numCells[2];
  
  bool* __valid = NULL;
  int* cellX = NULL;
  int* cellY = NULL;
  int* cellZ = NULL;
  FieldData* position = NULL;
  FieldData* density = NULL;
  FieldData* particleTemperature = NULL;
  bool* tracking = NULL;
  
  ByteOffset __validStride[1];
  ByteOffset cellXStride[1];
  ByteOffset cellYStride[1];
  ByteOffset cellZStride[1];
  ByteOffset positionStride[1];
  ByteOffset densityStride[1];
  ByteOffset particleTemperatureStride[1];
  ByteOffset trackingStride[1];
  
  
  accessParticleData(particles, particles_fields, __valid, __validStride, cellX, cellXStride,
                     cellY, cellYStride, cellZ, cellZStride, position, positionStride,
                     density, densityStride, particleTemperature, particleTemperatureStride,
                     tracking, trackingStride, runtime);
  
  if(writeFiles(timeStepNumber)) {
    std::string particlesFileName = dataFileName("./out/particles", timeStepNumber, bounds);
    writeParticlesToFile(particlesFileName, __valid, cellX, cellY, cellZ, position, density, particleTemperature, tracking,
                         __validStride, cellXStride, cellYStride, cellZStride, positionStride,
                         densityStride, particleTemperatureStride, trackingStride, runtime);
  }
  
#endif
  
  GLfloat* rgbaBuffer = NULL;
  GLfloat* depthBuffer = NULL;

  
#ifdef USE_SOFTWARE_RENDERING
  OSMesaContext mesaCtx;
  createGraphicsContext(mesaCtx, rgbaBuffer);
#else
  EGLContext eglCtx;
  EGLDisplay eglDpy;
  createGraphicsContext(eglCtx, eglDpy);
#endif
  
  
#ifdef STANDALONE
  
  render_image(width, height, centerCoordinates, velocity, temperature, totalCells, particleFilePath,
               &rgbaBuffer, &depthBuffer, numCells);
  write_ppm(outputFileName.c_str(), rgbaBuffer, width, height);
  std::string depthFileName = outputFileName + ".depth.ppm";
  
#else
  
  render_image(width, height, centerCoordinates, velocity, temperature, totalCells,
               __valid, position, density, particleTemperature, tracking,
               __validStride, positionStride, densityStride, particleTemperatureStride, trackingStride,
               &rgbaBuffer, &depthBuffer, runtime, numCells);
  
  if(writeFiles(timeStepNumber)) {
    write_ppm(imageFileName("./out/image", ".ppm", timeStepNumber, bounds).c_str(), rgbaBuffer, width, height);
    std::string depthFileName = imageFileName("./out/depth", ".zzz", timeStepNumber, bounds);
    FILE* depthFile = fopen(depthFileName.c_str(), "w");
    assert(depthFile != NULL);
    fprintf(depthFile, "%d %d\n", width, height);
    fwrite(depthBuffer, sizeof(GLfloat), width * height, depthFile);
    fclose(depthFile);
    std::cout << "wrote " << depthFileName << std::endl;
  }
  
  writeRenderedPixelsToImageFragments(rgbaBuffer, depthBuffer, runtime,
                                      imageFragment, imageFragment_fields, width, height);
  
  
#endif
  
  
  /* free the image buffer */
  free(rgbaBuffer);
  free(depthBuffer);
  
#ifdef USE_SOFTWARE_RENDERING
  destroyGraphicsContext(mesaCtx);
#else
  destroyGraphicsContext(eglDpy);
#endif
  
}



#ifndef STANDALONE



inline void compositePixelsLess(GLfloat *r0,
                                ByteOffset strideR0[3],
                                GLfloat *g0,
                                ByteOffset strideG0[3],
                                GLfloat *b0,
                                ByteOffset strideB0[3],
                                GLfloat *a0,
                                ByteOffset strideA0[3],
                                GLfloat *z0,
                                ByteOffset strideZ0[3],
                                GLfloat *userData0,
                                ByteOffset strideUserData0[3],
                                GLfloat *r1,
                                ByteOffset strideR1[3],
                                GLfloat *g1,
                                ByteOffset strideG1[3],
                                GLfloat *b1,
                                ByteOffset strideB1[3],
                                GLfloat *a1,
                                ByteOffset strideA1[3],
                                GLfloat *z1,
                                ByteOffset strideZ1[3],
                                GLfloat *userData1,
                                ByteOffset strideUserData1[3],
                                GLfloat *rOut,
                                ByteOffset strideROut[3],
                                GLfloat *gOut,
                                ByteOffset strideGOut[3],
                                GLfloat *bOut,
                                ByteOffset strideBOut[3],
                                GLfloat *aOut,
                                ByteOffset strideAOut[3],
                                GLfloat *zOut,
                                ByteOffset strideZOut[3],
                                GLfloat *userDataOut,
                                ByteOffset strideUserDataOut[3],
                                int numPixels){
  
  for(int i = 0; i < numPixels; ++i) {
    if(*z0 < *z1) {
      *rOut = *r0; *gOut = *g0; *bOut = *b0; *aOut = *a0; *zOut = *z0; *userDataOut = *userData0;
    } else {
      *rOut = *r1; *gOut = *g1; *bOut = *b1; *aOut = *a1; *zOut = *z1; *userDataOut = *userData1;
    }
    
    r0 += strideR0[0].offset / sizeof(*r0);
    g0 += strideG0[0].offset / sizeof(*g0);
    b0 += strideB0[0].offset / sizeof(*b0);
    a0 += strideA0[0].offset / sizeof(*a0);
    z0 += strideZ0[0].offset / sizeof(*z0);
    userData0 += strideUserData0[0].offset / sizeof(*userData0);
    
    r1 += strideR1[0].offset / sizeof(*r1);
    g1 += strideG1[0].offset / sizeof(*g1);
    b1 += strideB1[0].offset / sizeof(*b1);
    a1 += strideA1[0].offset / sizeof(*a1);
    z1 += strideZ1[0].offset / sizeof(*z1);
    userData1 += strideUserData1[0].offset / sizeof(*userData1);
    
    rOut += strideROut[0].offset / sizeof(*rOut);
    gOut += strideGOut[0].offset / sizeof(*gOut);
    bOut += strideBOut[0].offset / sizeof(*bOut);
    aOut += strideAOut[0].offset / sizeof(*aOut);
    zOut += strideZOut[0].offset / sizeof(*zOut);
    userDataOut += strideUserDataOut[0].offset / sizeof(*userDataOut);
    
  }
  
}


static void extractPixelsToRGBZBuffer(int numPixels,
                                      GLfloat *r0,
                                      ByteOffset strideR0[3],
                                      GLfloat *g0,
                                      ByteOffset strideG0[3],
                                      GLfloat *b0,
                                      ByteOffset strideB0[3],
                                      GLfloat *z0,
                                      ByteOffset strideZ0[3],
                                      GLfloat* RGBZ) {
  // could we speed this up by doing a block move of rgba and then overwriting a with z?
  
  for(int i = 0; i < numPixels; ++i) {
    *RGBZ++ = *r0;
    r0 += strideR0[0].offset / sizeof(*r0);
    *RGBZ++ = *g0;
    g0 += strideG0[0].offset / sizeof(*g0);
    *RGBZ++ = *b0;
    b0 += strideB0[0].offset / sizeof(*b0);
    *RGBZ++ = *z0;
    z0 += strideZ0[0].offset / sizeof(*z0);
  }
}



static void compositeGPU(GLfloat* RGBZ) {
  
  char* vertexShaderSource =
  "attribute vec2 vertexIn; \n
  varying vec2 textureCoord; \n
  void main() { \n
    textureCoord = vertexIn.xy; \n
    gl_Position = vec4(vertexIn.xy,0.0,1.0); \n
  }";
  
  char* fragmentShaderSource =
  "varying vec2 textureCoord; \n
  uniform sampler2D texture0; \n
  uniform sampler2D texture1; \n
  void main() { \n
    vec4 color0 = texture2D(texture0, textureCoord); \n
    vec4 color1 = texture2D(texture1, textureCoord); \n
    if(color0.w < color1.w) { \n
      gl_FragColor = color0; \n
    } else { \n
      gl_FragColor = color1; \n
    } \n
  }";
  
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  glCompileShader(vertexShader);

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  glCompileShader(fragmentShader);
  
  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  
  glLinkProgram(shaderProgram);
  glUseProgram(shaderProgram);

  GLfloat quadVertices[] = {
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f,
  };

  GLuint vbo;
  glGenBuffers(1, &vbo); // Generate 1 buffer
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);
  
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glFinish();

}



inline void compositePixelsGPU(GLfloat *r0,
                                ByteOffset strideR0[3],
                                GLfloat *g0,
                                ByteOffset strideG0[3],
                                GLfloat *b0,
                                ByteOffset strideB0[3],
                                GLfloat *a0,
                                ByteOffset strideA0[3],
                                GLfloat *z0,
                                ByteOffset strideZ0[3],
                                GLfloat *userData0,
                                ByteOffset strideUserData0[3],
                                GLfloat *r1,
                                ByteOffset strideR1[3],
                                GLfloat *g1,
                                ByteOffset strideG1[3],
                                GLfloat *b1,
                                ByteOffset strideB1[3],
                                GLfloat *a1,
                                ByteOffset strideA1[3],
                                GLfloat *z1,
                                ByteOffset strideZ1[3],
                                GLfloat *userData1,
                                ByteOffset strideUserData1[3],
                                GLfloat *rOut,
                                ByteOffset strideROut[3],
                                GLfloat *gOut,
                                ByteOffset strideGOut[3],
                                GLfloat *bOut,
                                ByteOffset strideBOut[3],
                                GLfloat *aOut,
                                ByteOffset strideAOut[3],
                                GLfloat *zOut,
                                ByteOffset strideZOut[3],
                                GLfloat *userDataOut,
                                ByteOffset strideUserDataOut[3],
                                int numPixels){
  // 1. extract pixels into RGBZ buffers
  GLfloat *RGBZ0 = (GLfloat*)calloc(width * height, sizeof(GLfloat));
  extractPixelsToRGBZBuffer(numPixels, r0, strideR0, g0, strideG0, b0, strideB0, z0, strideZ0, RGBZ0);
  GLfloat *RGBZ1 = (GLfloat*)calloc(width * height, sizeof(GLfloat));
  extractPixelsToRGBZBuffer(numPixels, r1, strideR1, g1, strideG1, b1, strideB1, z1, strideZ1, RGBZ1);
  
  // 2. upload both buffers to GPU
  unsigned int textures[2];
  glGenTextures(2, textures);
  glBindTexture(GL_TEXTURE_2D, textures[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, RGBZ0);
  glBindTexture(GL_TEXTURE_2D, textures[1]);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, RGBZ1);
  
  // 3. composite the result
  compositeGPU(RGBZ0);
  // 4. read back the result
  
  free(RGBZ0);
  free(RGBZ1);
}



void cxx_reduce(legion_runtime_t runtime_,
                legion_physical_region_t *leftSubregion,
                legion_field_id_t *leftSubregion_fields,
                legion_physical_region_t *rightSubregion,
                legion_field_id_t *rightSubregion_fields,
                int treeLevel,
                int offset) {
  
  Runtime *runtime = CObjectWrapper::unwrap(runtime_);
  
  PhysicalRegion* leftImage = CObjectWrapper::unwrap(leftSubregion[0]);
  std::vector<legion_field_id_t> fields;
  leftImage->get_fields(fields);
  const int expectedNumFields = 6;
  assert(fields.size() == expectedNumFields);
  Domain leftIndexSpaceDomain = runtime->get_index_space_domain(leftImage->get_logical_region().get_index_space());
  Rect<3> leftBounds = leftIndexSpaceDomain.get_rect<3>();
  
  // early out on empty regions
  
  if(leftBounds.lo.x[0] > leftBounds.hi.x[0]) {
    return;
  }
  
  float* leftR = NULL;
  float* leftG = NULL;
  float* leftB = NULL;
  float* leftA = NULL;
  float* leftZ = NULL;
  float* leftUserData = NULL;
  
  float* rightR = NULL;
  float* rightG = NULL;
  float* rightB = NULL;
  float* rightA = NULL;
  float* rightZ = NULL;
  float* rightUserData = NULL;
  
  ByteOffset leftStrideR[3];
  ByteOffset leftStrideG[3];
  ByteOffset leftStrideB[3];
  ByteOffset leftStrideA[3];
  ByteOffset leftStrideZ[3];
  ByteOffset leftStrideUserData[3];
  
  ByteOffset rightStrideR[3];
  ByteOffset rightStrideG[3];
  ByteOffset rightStrideB[3];
  ByteOffset rightStrideA[3];
  ByteOffset rightStrideZ[3];
  ByteOffset rightStrideUserData[3];
  
  
  for(unsigned field = 0; field < fields.size(); ++field) {
    PhysicalRegion* leftImage = CObjectWrapper::unwrap(leftSubregion[field]);
    PhysicalRegion* rightImage = CObjectWrapper::unwrap(rightSubregion[field]);
    switch(field) {
      case 0:
        create_field_pointer(*leftImage, leftR, leftSubregion_fields[field], leftStrideR, runtime);
        create_field_pointer(*rightImage, rightR, rightSubregion_fields[field], rightStrideR, runtime);
        break;
      case 1:
        create_field_pointer(*leftImage, leftG, leftSubregion_fields[field], leftStrideG, runtime);
        create_field_pointer(*rightImage, rightG, rightSubregion_fields[field], rightStrideG, runtime);
        break;
      case 2:
        create_field_pointer(*leftImage, leftB, leftSubregion_fields[field], leftStrideB, runtime);
        create_field_pointer(*rightImage, rightB, rightSubregion_fields[field], rightStrideB, runtime);
        break;
      case 3:
        create_field_pointer(*leftImage, leftA, leftSubregion_fields[field], leftStrideA, runtime);
        create_field_pointer(*rightImage, rightA, rightSubregion_fields[field], rightStrideA, runtime);
        break;
      case 4:
        create_field_pointer(*leftImage, leftZ, leftSubregion_fields[field], leftStrideZ, runtime);
        create_field_pointer(*rightImage, rightZ, rightSubregion_fields[field], rightStrideZ, runtime);
        break;
      case 5:
        create_field_pointer(*leftImage, leftUserData, leftSubregion_fields[field], leftStrideUserData, runtime);
        create_field_pointer(*rightImage, rightUserData, rightSubregion_fields[field], rightStrideUserData, runtime);
        break;
    }
  }
  
#ifdef USE_SOFTWARE_RENDERING
  
  compositePixelsLess(leftR, leftStrideR, leftG, leftStrideG, leftB, leftStrideB, leftA, leftStrideA, leftZ, leftStrideZ, leftUserData, leftStrideUserData,
                      rightR, rightStrideR, rightG, rightStrideG, rightB, rightStrideB, rightA, rightStrideA, rightZ, rightStrideZ, rightUserData, rightStrideUserData,
                      leftR, leftStrideR, leftG, leftStrideG, leftB, leftStrideB, leftA, leftStrideA, leftZ, leftStrideZ, leftUserData, leftStrideUserData,
                      (int)leftBounds.volume());
  
#else
  
  compositePixelsGPU(leftR, leftStrideR, leftG, leftStrideG, leftB, leftStrideB, leftA, leftStrideA, leftZ, leftStrideZ, leftUserData, leftStrideUserData,
                     rightR, rightStrideR, rightG, rightStrideG, rightB, rightStrideB, rightA, rightStrideA, rightZ, rightStrideZ, rightUserData, rightStrideUserData,
                     leftR, leftStrideR, leftG, leftStrideG, leftB, leftStrideB, leftA, leftStrideA, leftZ, leftStrideZ, leftUserData, leftStrideUserData,
                     (int)leftBounds.volume());
  
#endif
  
}





void cxx_saveImage(legion_runtime_t runtime_,
                   int width,
                   int height,
                   int timeStepNumber,
// CODEGEN: legion_physical_region_t_imageFragmentX
)
{
  // CODEGEN: legion_physical_region_t_imageFragment_arrays
  
  Runtime *runtime = CObjectWrapper::unwrap(runtime_);
  
  PhysicalRegion* fragment = CObjectWrapper::unwrap(imageFragment0[0]);
  std::vector<legion_field_id_t> fields;
  fragment->get_fields(fields);
  const int expectedNumFields = 6;
  assert(fields.size() == expectedNumFields);
  
  size_t numElements = width * height * expectedNumFields;
  GLfloat* rgbaBuffer = (GLfloat*)calloc(numElements, sizeof(GLfloat));
  GLfloat* rgba = rgbaBuffer;
  
  float* R = NULL;
  float* G = NULL;
  float* B = NULL;
  float* A = NULL;
  float* Z = NULL;
  float* UserData = NULL;
  
  ByteOffset strideR[3];
  ByteOffset strideG[3];
  ByteOffset strideB[3];
  ByteOffset strideA[3];
  ByteOffset strideZ[3];
  ByteOffset strideUserData[3];
  
  unsigned row = 0;
  unsigned fragmentID = 0;
  while(row < (unsigned)height) {
    
    PhysicalRegion* fragment = CObjectWrapper::unwrap(imageFragment[fragmentID][0]);
    for(unsigned field = 0; field < fields.size(); ++field) {
      switch(field) {
        case 0:
          create_field_pointer(*fragment, R, imageFragment_fields[fragmentID][field], strideR, runtime);
          break;
        case 1:
          create_field_pointer(*fragment, G, imageFragment_fields[fragmentID][field], strideG, runtime);
          break;
        case 2:
          create_field_pointer(*fragment, B, imageFragment_fields[fragmentID][field], strideB, runtime);
          break;
        case 3:
          create_field_pointer(*fragment, A, imageFragment_fields[fragmentID][field], strideA, runtime);
          break;
        case 4:
          create_field_pointer(*fragment, Z, imageFragment_fields[fragmentID][field], strideZ, runtime);
          break;
        case 5:
          create_field_pointer(*fragment, UserData, imageFragment_fields[fragmentID][field], strideUserData, runtime);
          break;
      }
    }
    int pixelCount = 0;
    Domain indexSpaceDomain = runtime->get_index_space_domain(fragment->get_logical_region().get_index_space());
    Rect<3> bounds = indexSpaceDomain.get_rect<3>();
    
    for(unsigned y = (unsigned)bounds.lo.x[1]; y <= bounds.hi.x[1]; ++y) {
      for(unsigned x = (unsigned)bounds.lo.x[0]; x <= bounds.hi.x[0]; ++x) {
        rgba[0] = *R;
        rgba[1] = *G;
        rgba[2] = *B;
        rgba[3] = *A;
        rgba += 4;
        R += strideR[0].offset / sizeof(*R);
        G += strideG[0].offset / sizeof(*G);
        B += strideB[0].offset / sizeof(*B);
        A += strideA[0].offset / sizeof(*A);
        pixelCount++;
      }
      row++;
    }
    
    fragmentID++;
  }
  
  // in the future, this displays on a projector tile
  write_ppm(imageFileName("./out/image", ".ppm", timeStepNumber).c_str(), rgbaBuffer, width, height);
  
  free(rgbaBuffer);
}





#endif




#ifdef STANDALONE//offline development

static
void readCellData(std::string filePath,
                  FieldData* &centerCoordinates,
                  FieldData* &velocity,
                  FieldData* &temperature,
                  int &totalCells,
                  int numCells[3],
                  FieldData min[3],
                  FieldData max[3]) {
  std::ifstream inputFile(filePath.c_str(), std::ifstream::in);
  
  if (inputFile.is_open()) {
    std::string inputLine;
    if(getline(inputFile, inputLine)) {
      unsigned expectedNumCells;
      unsigned xLo, yLo, zLo, xHi, yHi, zHi;
      int numFields = sscanf(inputLine.c_str(), "[(%d,%d,%d),(%d,%d,%d)]",
                             &xLo, &yLo, &zLo, &xHi, &yHi, &zHi);
      if(numFields == 6) {
        numCells[0] = xHi - xLo + 1;
        numCells[1] = yHi - yLo + 1;
        numCells[2] = zHi - zLo + 1;
        expectedNumCells = numCells[0] * numCells[1] * numCells[2];
      } else {
        sscanf(inputLine.c_str(), "%d", &expectedNumCells);
        numCells[0] = expectedNumCells;
        numCells[1] = 1;
        numCells[2] = 1;
      }
      centerCoordinates = new FieldData[expectedNumCells * 3];
      velocity = new FieldData[expectedNumCells * 3];
      temperature = new FieldData[expectedNumCells];
      
      FieldData* cc = centerCoordinates;
      FieldData* v = velocity;
      FieldData* t = temperature;
      unsigned cellCount = 0;
      min[0] = 999999.0;
      min[1] = 999999.0;
      min[2] = 999999.0;
      max[0] = -min[0];
      max[1] = -min[1];
      max[2] = -min[2];
      
      while (getline(inputFile, inputLine)) {
        unsigned serialID;
        unsigned x, y, z;
        sscanf(inputLine.c_str(), "%d %d %d %d  %lf %lf %lf %lf %lf %lf %lf",
               &serialID, &x, &y, &z,
               cc, cc + 1, cc + 2,
               v, v + 1, v + 2,
               t);
        for(unsigned j = 0; j < 3; ++j) {
          min[j] = (min[j] > cc[j]) ? cc[j] : min[j];
          max[j] = (max[j] < cc[j]) ? cc[j] : max[j];
        }
        
        cc += 3;
        v += 3;
        t++;
        cellCount++;
      }
      inputFile.close();
      totalCells = cellCount;
      if(cellCount == expectedNumCells) {
        std::cout << "loaded " << cellCount << " cells" << std::endl;
      } else {
        std::cerr << "expected " << expectedNumCells << " cells but loaded " << cellCount << std::endl;
      }
    }
  } else {
    std::cerr << "Unable to open file " << filePath << std::endl;
  }
  
  std::cout << "cell min " << min[0] << "," << min[1] << "," << min[2] << std::endl;
  std::cout << "cell max " << max[0] << "," << max[1] << "," << max[2] << std::endl;
  
}




int main(const int argc, const char *argv[]) {
  if(argc != 4) {
    std::cout << "usage: " << argv[0] << " <cellfile> <particlefile> <outputimage>" << std::endl;
    std::cout << "e.g. " << argv[0] << "cells.00007.0_0_0__127_127_255.txt	particles.00007.0_0_0__127_127_255.txt image.ppm" << std::endl;
    exit(-1);
  }
  
  std::string cellFilePath = argv[1];
  std::string particleFilePath = argv[2];
  std::string outputFilePath = argv[3];
  
  std::cout << "reading cell data from " << cellFilePath << std::endl;
  readCellData(cellFilePath, centerCoordinates, velocity, temperature, totalCells, numCells, min, max);
  
  std::cout << "reading particle data from " << particleFilePath << std::endl;
  
  std::cout << "calling cxx_render with output to " << outputFilePath << std::endl;
  cxx_render(particleFilePath, outputFilePath, numCells);
  return 0;
}

#endif



