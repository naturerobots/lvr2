/*
 * PlyStructs.h
 *
 *  Created on: 21.10.2008
 *      Author: twiemann
 */

#ifndef PLYSTRUCTS_H_
#define PLYSTRUCTS_H_

#define PLY_LITTLE_ENDIAN "format binary_little_endian 1.0\n"
#define PLY_BIG_ENDIAN "format binary_big_endian 1.0\n"

struct PlyHeaderDescription{

  char ply[5] ;
  char format[50];
  char comment[256];

};

struct PlyVertexDescription{

  char element[25];
  unsigned int count;
  char property_x[25];
  char property_y[25];
  char property_z[25];
  char property_nx[25];
  char property_ny[25];
  char property_nz[25];
};

struct PlyFaceDescription{

  char face[10];
  unsigned int count;
  char property[40];

};

struct PlyVertex{
  float x;
  float y;
  float z;
  float nx;
  float ny;
  float nz;
  float r;
  float g;
  float b;

  float u;
  float v;

  int texture;
};

struct PlyFace{
  int vertexCount;
  int indices[3];
};


#endif /* PLYSTRUCTS_H_ */
