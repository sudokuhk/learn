#ifndef __SHADER_H__
#define __SHADER_H__

#include <OpenGLES/gltypes.h>
#include <map>

class CShader
{
public:
    CShader(void);
    
    ~CShader(void);
    
    bool Load(const char * vertex, const char * fragment);
    
    void AddAttribute(const char * attribute, GLuint index);
    
    GLuint GetAttributeLocation(const char * attribute) const;
    
    bool Link(void);
    
    void Use(void);
    
    void SetInt(const char * uniform_name, int value);
    
    void Unload(void);
    
private:
    GLuint _CreateShader(const char * sz_shader, int type);
    
    void _DeleteProgram(GLuint & program);
    
    void _DeleteShader(GLuint & shader);
    
    GLuint _GetUniformLocation(const char * uniform_key);
private:
    GLuint _program;
    
    typedef std::map<std::string, GLuint> MAP;
    MAP _parameters;
    MAP _attributes;
};

#endif /* __SHADER_H__ */
