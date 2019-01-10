#include "Shader.h"
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#include <stdio.h>

CShader::CShader(void) : _program(0)
{
}

CShader::~CShader(void)
{
    _DeleteProgram(_program);
    _parameters.clear();
}

bool CShader::Load(const char * vertex, const char * fragment)
{
    if (vertex == NULL || fragment == NULL) {
        return false;
    }
    
    _DeleteShader(_program);
    
    GLuint vertexSharder = _CreateShader(vertex, GL_VERTEX_SHADER);
    GLuint fragmentShader = _CreateShader(fragment, GL_FRAGMENT_SHADER);
    
    if (vertexSharder > 0 || fragmentShader > 0) {
        _program = glCreateProgram();
        glAttachShader(_program, vertexSharder);
        glAttachShader(_program, fragmentShader);
    }
    
    _DeleteShader(vertexSharder);
    _DeleteShader(fragmentShader);
    _parameters.clear();
    _attributes.clear();
    
    return _program > 0;
}

void CShader::AddAttribute(const char * attribute, GLuint index)
{
    const std::string key(attribute);
    MAP::iterator it = _attributes.find(key);
    if (it == _attributes.end()) {
        glBindAttribLocation(_program, index, attribute);
        _attributes[key] = index;
    } else {
        if (it->second != index) {
            glBindAttribLocation(_program, index, attribute);
            it->second = index;
        }
    }
}

GLuint CShader::GetAttributeLocation(const char * attribute) const
{
    const std::string key(attribute);
    MAP::const_iterator it = _attributes.find(key);
    if (it != _attributes.end()) {
        return it->second;
    }
    return 0;
}

bool CShader::Link(void)
{
    if (_program == 0) {
        return false;
    }
    
    GLint success = 0;
    glLinkProgram(_program);
    glGetProgramiv(_program, GL_LINK_STATUS, &success);
    if (success != GL_TRUE) {
        char log[1024];
        glGetProgramInfoLog(_program, 1024, NULL, log);
        _DeleteProgram(_program);
    }
    return _program > 0;
}

void CShader::Use(void)
{
    if (_program > 0) {
        glUseProgram(_program);
    }
}

void CShader::SetInt(const char * uniform_name, int value)
{
    if (uniform_name == NULL) {
        return;
    }
    
    if (_program > 0) {
        GLuint location = _GetUniformLocation(uniform_name);
        glUniform1i(location, (GLint)value);
    } else {
        _parameters.clear();
    }
}

void CShader::Unload(void)
{
    _DeleteProgram(_program);
}

GLuint CShader::_CreateShader(const char * sz_shader, int type)
{
    GLuint shader = 0;
    GLint success = 0;
    
    shader = glCreateShader(type);
    glShaderSource(shader, 1, &sz_shader, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE) {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 0) {
            char* buf = (char*) malloc(infoLen);
            if (buf != NULL) {
                glGetShaderInfoLog(shader, infoLen, NULL, buf);
                free(buf);
            }
            _DeleteShader(shader);
        }
    }
    return shader;
}

void CShader::_DeleteProgram(GLuint & program)
{
    if (program > 0) {
        glDeleteProgram(program);
        program = 0;
    }
}

void CShader::_DeleteShader(GLuint & shader)
{
    if (shader > 0) {
        glDeleteShader(shader);
        shader = 0;
    }
}

GLuint CShader::_GetUniformLocation(const char * uniform_key)
{
    const std::string key(uniform_key);
    MAP::const_iterator it = _parameters.find(key);
    GLuint location = 0;
    if (it == _parameters.end()) {
        location = glGetUniformLocation(_program, uniform_key);
        _parameters[key] = location;
    } else {
        location = it->second;
    }
    return location;
}

