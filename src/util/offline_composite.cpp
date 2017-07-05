//
//  main.cpp
//  offline_composite
//
//  Created by Alan Heirich on 7/2/17.
//  Copyright © 2017 Alan Heirich. All rights reserved.
//

#include <malloc.h>
#include <stdio.h>
#include <iostream>
#include <assert.h>

class FrameBuffer {
public:
  FrameBuffer() : mValid(false), mRGBA(NULL), mDepth(NULL) {}
  
  FrameBuffer(std::string imageFilePath, std::string depthFilePath) {
    mImageFilePath = imageFilePath;
    mDepthFilePath = depthFilePath;
    mValid = true;
    loadDepth(mDepthFilePath, mDepth, mWidth, mHeight);
    loadRGBA(mImageFilePath, mRGBA, mWidth, mHeight);
    saveGrayMap(mDepthFilePath, mDepth, mWidth, mHeight);
  }
  
  virtual ~FrameBuffer() {
    if(mRGBA != NULL) free(mRGBA);
    if(mDepth != NULL) free(mDepth);
  }
  
  bool valid() const { return mValid; }
  float* RGBA() const { return mRGBA; }
  float* depth() const { return mDepth; }
  unsigned width() const { return mWidth; }
  unsigned height() const { return mHeight; }
  
  
  void writeDepth(std::string filePath) {
    FILE* f = fopen(filePath.c_str(), "wb");
    assert(f != NULL);
    // no quite ppm format TODO
    fprintf(f, "%d %d\n", mWidth, mHeight);
    fwrite(mDepth, sizeof(float), mWidth * mHeight, f);
    fclose(f);
    std::cout << "wrote " << filePath << std::endl;
  }
  
  void writeRGBA(std::string filePath) {
    FILE* f = fopen(filePath.c_str(), "wb");
    assert(f != NULL);
    fprintf(f, "P6\n# %s\n%d %d\n255\n", filePath.c_str(), mWidth, mHeight);
    fclose(f);
    
    unsigned char* buffer = (unsigned char*) calloc(mWidth * mHeight, 3 * sizeof(unsigned char));
    unsigned char* b = buffer;
    for(int y = mHeight - 1; y >= 0; y--) {
      float *rgba = mRGBA + (y * mWidth * 4);
      for(unsigned x = 0; x < mWidth; ++x) {
        b[0] = 0xff & (unsigned)std::min((rgba[0] * 255.0f), 255.0f);
        b[1] = 0xff & (unsigned)std::min((rgba[1] * 255.0f), 255.0f);
        b[2] = 0xff & (unsigned)std::min((rgba[2] * 255.0f), 255.0f);
        b += 3;
        rgba += 4;
      }
    }
    
    f = fopen(filePath.c_str(), "ab");
    fwrite(buffer, sizeof(unsigned char), mWidth * mHeight * 3 * sizeof(unsigned char), f);
    free(buffer);
    fclose(f);
    std::cout << "wrote " << filePath << std::endl;
  }
  
  
  
private:
  
  void saveGrayMap(std::string filePath, float* depth, int width, int height) {
    std::string fullFilePath = filePath + ".pgm";
    FILE* f = fopen(fullFilePath.c_str(), "w");
    assert(f != NULL);
    float* d = depth;
    fprintf(f, "P5 %d %d 255\n", width, height);
    for(unsigned y = 0; y < height; ++y) {
      unsigned char buffer[width];
      unsigned char *b = buffer;
      for(unsigned x = 0; x < width; ++x) {
        float value = std::min(std::max(255.0f * *d, 0.0f), 255.0f);
        *b = ((unsigned)value) & 0xff;
        b++;
        d++;
      }
      fwrite(buffer, 1, width, f);
    }
    fclose(f);
  }
  
  void loadRGBA(std::string filePath, float* &rgba, unsigned width, unsigned height) {
    FILE* f = fopen(filePath.c_str(), "rb");
    assert(f != NULL);
    char line[256];
    // ppm format
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);
    unsigned fileWidth;
    unsigned fileHeight;
    sscanf(line, "%d %d", &fileWidth, &fileHeight);
    assert(fileWidth == width && fileHeight == height);
    fgets(line, sizeof(line), f);
    
    int bufferSize = width * height * 3 * sizeof(unsigned char);
    unsigned char* buffer = (unsigned char*) calloc(bufferSize, 1);
    fread(buffer, bufferSize, 1, f);
    fclose(f);
    int rgbaSize = width * height * 4 * sizeof(float);
    rgba = (float*) calloc(rgbaSize, 1);
    unsigned char* b = buffer;
    for(int y = height - 1; y >= 0; y--) {
      float* r = rgba + (y * width * 4);
      for(unsigned x = 0; x < width; ++x) {
        r[0] = (float)(b[0]) / 255.0f;
        r[1] = (float)(b[1]) / 255.0f;
        r[2] = (float)(b[2]) / 255.0f;
        r[3] = 1.0f;
        r += 4;
        b += 3;
      }
    }
    free(buffer);
    std::cout << "loaded image " << width << "," << height << " from " << filePath << std::endl;
  }
  
  void loadDepth(std::string filePath, float* &depth, unsigned &width, unsigned &height) {
    FILE* f = fopen(filePath.c_str(), "r");
    assert(f != NULL);
    char buffer[256];
    // not quite ppm format despite the extension
    fgets(buffer, sizeof(buffer), f);
    sscanf(buffer, "%d %d", &width, &height);
    depth = (float*) calloc(width * height, sizeof(float));
    fread(depth, sizeof(float), width * height, f);
    fclose(f);
    std::cout << "loaded depth map " << width << "," << height << " from " << filePath << std::endl;
  }
  
  std::string mImageFilePath;
  std::string mDepthFilePath;
  bool mValid;
  unsigned mWidth;
  unsigned mHeight;
  float* mDepth;
  float* mRGBA;
};

static void reduce(FrameBuffer* source0, FrameBuffer* source1, FrameBuffer* result) {
  float* rgba0 = source0->RGBA();
  float* depth0 = source0->depth();
  float* rgba1 = source1->RGBA();
  float* depth1 = source1->depth();
  float* rgbaResult = result->RGBA();
  float* depthResult = result->depth();
  unsigned foreground = 0;
  unsigned background = 0;
  
  std::cout << " reduce " << source0->width() << "," << source0->height() << std::endl;
  for(unsigned y = 0; y < source0->height(); ++y) {
    for(unsigned x = 0; x < source0->width(); ++x) {
      
      if(*depth0 < *depth1) {
        for(unsigned i = 0; i < 4; ++i) {
          rgbaResult[i] = rgba0[i];
        }
        *depthResult = *depth0;
        foreground++;
      } else {
        for(unsigned i = 0; i < 4; ++i) {
          rgbaResult[i] = rgba1[i];
        }
        *depthResult = *depth1;
        background++;
      }
      rgba0 += 4;
      rgba1 += 4;
      rgbaResult += 4;
      depth0++;
      depth1++;
      depthResult++;
    }
  }
  std::cout << "foreground " << foreground << " background " << background << std::endl;
}


int main(int argc, const char * argv[]) {
  
  std::cout << argv[0] << " <outputFile>.ppm <infile0>.ppm <infile1>.ppm ..." << std::endl;
  
  assert(argc >= 4);
  const char* outputFileName = argv[1];
  const char** inputFileName = argv + 2;
  FrameBuffer* source0 = NULL;
  const int numInputs = argc - 2;

  for(int i = 0; i < numInputs; i += 2) {
    std::string imageFileName = inputFileName[i];
    std::string depthFileName = inputFileName[i + 1];
    
    FrameBuffer* source1 = new FrameBuffer(imageFileName, depthFileName);
    if(source0 != NULL && source0->valid() && source1->valid()) {
      reduce(source0, source1, source1);
    }
    
    source0 = source1;
    if(i + 2 >= numInputs) {
      source1->writeRGBA(outputFileName);
    }
  }
  
  return 0;
}
