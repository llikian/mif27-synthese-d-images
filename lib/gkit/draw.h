
#ifndef _DRAW_H
#define _DRAW_H

#include "glcore.h"
#include "color.h"
#include "mat.h"
#include "mesh_io.h"


void default_color( const Color& color );
void default_texture( const unsigned unit, const GLuint texture );

void draw( const GLuint vao, 
    const GLenum primitives, const unsigned count, 
    const Transform& model, const Transform& view, const Transform& projection );

void draw( const GLuint vao, 
    const GLenum primitives, MeshIOGroup& group, 
    const Transform& model, const Transform& view, const Transform& projection );
    
#endif

