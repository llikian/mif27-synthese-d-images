
#ifndef _DEFAULT_PROGRAM_H
#define _DEFAULT_PROGRAM_H

#include <vector>

#include "glcore.h"
#include "vec.h"
#include "color.h"

  
GLuint create_default_program( const GLenum primitives, 
    const std::vector<Point>& positions, const std::vector<unsigned>& indices= {},
    const std::vector<Point>& texcoords= {}, const std::vector<Vector>& normals= {}, const std::vector<Color>& colors= {} );

GLuint create_default_program( const GLenum primitives, 
    const size_t positions, const size_t texcoords= 0, const size_t normals= 0, const size_t colors= 0 );

#endif
