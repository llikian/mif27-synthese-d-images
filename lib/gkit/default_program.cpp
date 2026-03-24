
#include <cassert>

#include "program.h"
#include "default_program.h"


//! description d'un shader program compile.
struct PipelineProgram
{
    std::string name;
    std::string source;
    std::string definitions;
    GLuint program;
};

//! ensemble de shader programs compiles. singleton.
struct PipelineCache
{
    //! renvoie un shader program compile.
    PipelineProgram *find( const std::string& name, const std::string& source, const std::string& definitions= "" )
    {
        // todo peut etre remplacer par une unordered_map ? mais il y a peu de shaders utilise normalement...
        for(auto program : programs)
        {
            if(program->name == name && program->definitions == definitions)
                return program;
        }
        
        // cree le programme s'il n'existe pas deja...
        GLuint p= create_program(name, source, definitions);
        program_print_errors(p);
        
        PipelineProgram *program= new PipelineProgram { name, source, definitions, p };
        programs.push_back( program );
        return program;
    }
    
    ~PipelineCache( ) 
    {
        // le contexte est deja detruit lorsque ce destructeur est appelle... 
        // trop tard pour detruire les programs et les shaders...
        release();
    }
    
    void release( )
    {
        printf("[pipeline cache] %u programs.\n", unsigned(programs.size()));
        for(auto program : programs)
            release_program(program->program);
    }
    
    //! acces au singleton.
    static PipelineCache& manager( ) 
    {
        static PipelineCache cache;
        return cache;
    }
    
protected:
    //! constructeur prive. 
    PipelineCache( ) : programs() {}
    
    std::vector<PipelineProgram *> programs;
};


static const char *mesh_shader= R"(
#version 330

#ifdef VERTEX_SHADER
    layout(location= 0) in vec3 position;
    uniform mat4 mvpMatrix;
    
    #ifdef USE_TEXCOORD
        layout(location= 1) in vec2 texcoord;
        out vec2 vertex_texcoord;
    #endif
    
    #ifdef USE_NORMAL
        layout(location= 2) in vec3 normal;
        uniform mat4 normalMatrix;
        out vec3 vertex_normal;
    #endif
    
    uniform mat4 mvMatrix;
    out vec3 vertex_position;
    
    void main( )
    {
        gl_Position= mvpMatrix * vec4(position, 1);
        
    #ifdef USE_TEXCOORD
        vertex_texcoord= texcoord;
    #endif
    
    #ifdef USE_NORMAL
        vertex_normal= mat3(normalMatrix) * normal;
    #endif
    vertex_position= vec3(mvMatrix * vec4(position, 1));
    }
#endif


#ifdef FRAGMENT_SHADER
    #ifdef USE_TEXCOORD
        in vec2 vertex_texcoord;
        uniform sampler2D diffuse_color;
    #endif
    
    #ifdef USE_NORMAL
        in vec3 vertex_normal;
    #endif
    
    in vec3 vertex_position;

    uniform vec4 mesh_color= vec4(1, 1, 1, 1);

    out vec4 fragment_color;
    void main( )
    {
        vec4 color= mesh_color;

    #ifdef USE_TEXCOORD
        color= color * texture(diffuse_color, vertex_texcoord);
    #endif
        
        vec3 normal= vec3(0, 0, 1);
    #ifdef USE_NORMAL
        normal= vertex_normal;
    #else
        vec3 t= normalize(dFdx(vertex_position));
        vec3 b= normalize(dFdy(vertex_position));
        normal= cross(t, b);
    #endif

        float cos_theta= abs(dot(normalize(normal), normalize(-vertex_position)));
        
        color.rgb= color.rgb * cos_theta;
        fragment_color= color;
    }
#endif
)";

static const char *line_shader= R"(
#version 330

#ifdef VERTEX_SHADER
    layout(location= 0) in vec3 position;
    uniform mat4 mvpMatrix;
    
    #ifdef USE_COLOR
        layout(location= 3) in vec4 color;
        out vec4 vertex_color;
    #endif
    
    void main( )
    {
        #ifdef USE_COLOR
            vertex_color= color;
        #endif
        
        gl_Position= mvpMatrix * vec4(position, 1);
    }
#endif

#ifdef FRAGMENT_SHADER
    uniform vec4 line_color= vec4(1, 1, 1, 1);
    
    #ifdef USE_COLOR
        in vec4 vertex_color;
    #endif
    
    out vec4 fragment_color;
    
    void main( )
    {
    #ifdef USE_COLOR
        fragment_color= vertex_color;
    #else
        fragment_color= line_color;
    #endif
    }
#endif
)";
 


GLuint create_default_program( const GLenum primitives, const size_t positions, const size_t texcoords, const size_t normals, const size_t colors )
{
    assert(positions);
    
    std::string definitions;
    if(texcoords > 0 && texcoords == positions)
        definitions.append("#define USE_TEXCOORD\n");
    if(normals > 0 && normals == positions)
        definitions.append("#define USE_NORMAL\n");
    if(colors > 0 && colors == positions)
        definitions.append("#define USE_COLOR\n");
    
    PipelineProgram *program= nullptr;
    if(primitives == GL_TRIANGLES) 
        program= PipelineCache::manager().find("mesh", mesh_shader, definitions);
    else
        program= PipelineCache::manager().find("line", line_shader, definitions);
    
    assert(program);
    return program->program;
}


GLuint create_default_program( const GLenum primitives, 
    const std::vector<Point>& positions, const std::vector<unsigned>& indices,
    const std::vector<Point>& texcoords, const std::vector<Vector>& normals, const std::vector<Color>& colors )
{
    return create_default_program(primitives, positions.size(), texcoords.size(), normals.size(), colors.size());
}

