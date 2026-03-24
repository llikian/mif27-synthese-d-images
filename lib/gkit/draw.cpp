
#include <cassert>

#include "draw.h"
#include "program.h"
#include "uniforms.h"
#include "default_program.h"


Color draw_color= White();
void default_color( const Color& color )
{
    draw_color= color;
}

unsigned draw_unit= 0;
GLuint draw_texture= 0;
void default_texture( const unsigned unit, const GLuint texture )
{
    draw_unit= unit;
    draw_texture= texture;
}


static unsigned check_buffer( const int index )
{
    GLint buffer= 0;
    GLint status= 0;
    glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &status);
    if(status != GL_FALSE)
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &buffer);
    
    return buffer;
}


void draw( const GLuint vao, 
    const GLenum primitives, const unsigned count, 
    const Transform& model, const Transform& view, const Transform& projection )
{
    assert(vao > 0);
    glBindVertexArray(vao);
    
    // un peu de gymnastique, analyse le mesh pour recuperer les attributs des sommets
    unsigned positions= check_buffer(0);
    unsigned texcoords= check_buffer(1);
    unsigned normals= check_buffer(2);
    unsigned colors= check_buffer(3);
    
    // si les buffers existent, les attributs des sommets aussi, cf create_buffers()...
    GLuint program= create_default_program(primitives, positions ? 1 : 0, texcoords ? 1 : 0, normals ? 1 : 0, colors ? 1 : 0);
    glUseProgram(program);
    
    // transformations
    Transform mv= view * model;
    Transform normal= view * model;
    Transform mvp= projection * mv;
    program_uniform(program, "mvpMatrix", mvp);
    program_uniform(program, "mvMatrix", mv);
    program_uniform(program, "normalMatrix", normal);
    
    program_uniform(program, "mesh_color", draw_color);
    program_uniform(program, "line_color", draw_color);
    if(texcoords)
        program_use_texture(program, "diffuse_color", draw_unit, draw_texture);
    
    // mesh indexe, ou pas ?
    GLuint index_buffer= 0;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, (GLint *) &index_buffer);
    
    // draw !!
    if(index_buffer)
        glDrawElements(primitives, count, GL_UNSIGNED_INT, nullptr);
    else
        glDrawArrays(primitives, 0, count);    
}


void draw( const GLuint vao, 
    const GLenum primitives, MeshIOGroup& group, 
    const Transform& model, const Transform& view, const Transform& projection )
{
    assert(vao > 0);
    assert(primitives == GL_TRIANGLES);
    glBindVertexArray(vao);
    
    // un peu de gymnastique, analyse le mesh pour recuperer les attributs des sommets
    unsigned positions= check_buffer(0);
    unsigned texcoords= check_buffer(1);
    unsigned normals= check_buffer(2);
    unsigned colors= check_buffer(3);
    
    // si les buffers existent, les attributs des sommets aussi, cf create_buffers()...
    GLuint program= create_default_program(primitives, positions ? 1 : 0, texcoords ? 1 : 0, normals ? 1 : 0, colors ? 1 : 0);
    glUseProgram(program);
    
    // transformations
    Transform mv= view * model;
    Transform normal= view * model;
    Transform mvp= projection * mv;
    program_uniform(program, "mvpMatrix", mvp);
    program_uniform(program, "mvMatrix", mv);
    program_uniform(program, "normalMatrix", normal);
    
    program_uniform(program, "mesh_color", draw_color);
    program_uniform(program, "line_color", draw_color);
    if(texcoords)
        program_use_texture(program, "diffuse_color", draw_unit, draw_texture);
    
    // verifie que le mesh est bien indexe
    GLuint index_buffer= 0;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, (GLint *) &index_buffer);
    assert(index_buffer != 0);
    
    glDrawElements(GL_TRIANGLES, 3 * group.count, GL_UNSIGNED_INT, (void *)(3 * group.first * sizeof(unsigned)));
}

