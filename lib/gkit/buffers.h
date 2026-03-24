
#ifndef _BUFFERS_H
#define _BUFFERS_H

#include <vector>

#include "glcore.h"
#include "vec.h"
#include "color.h"


GLuint create_buffers( const std::vector<Point>& positions, const std::vector<unsigned>& indices= {},
    const std::vector<Point>& texcoords= {}, const std::vector<Vector>& normals= {}, const std::vector<Color>& colors= {} );

void update_buffers( const GLuint vao, 
    const std::vector<Point>& positions, const std::vector<unsigned>& indices= {},
    const std::vector<Point>& texcoords= {}, const std::vector<Vector>& normals= {}, const std::vector<Color>& colors= {} );

void release_buffers( const GLuint vao );

void check_buffers( const GLuint vao );

#endif
