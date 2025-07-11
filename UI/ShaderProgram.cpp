#include "ShaderProgram.h"

#include "../client/human/GGHumanClientApp.h"
#include "../util/Logger.h"


namespace {
    void CHECK_ERROR(const char* fn, const char* e) {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::string_view error_msg;
            switch(error) {
                case GL_INVALID_ENUM:      error_msg = "invalid enumerant"; break;
                case GL_INVALID_VALUE:     error_msg = "invalid value";     break;
                case GL_INVALID_OPERATION: error_msg = "invalid operation"; break;
            }
            ErrorLogger() << fn << " () : GL error on " << e << ": '" << error_msg << "'";
        }
    }

    void GetShaderLog(GLuint shader, std::string& log) {
        log.clear();
        GLint logSize{};
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, std::addressof(logSize));
        CHECK_ERROR("GetShaderLog", "glGetShaderiv(GL_INFO_LOG_LENGTH)");
        if (0 < logSize) {
            log.resize(logSize, '\0');
            GLsizei chars{};
            glGetShaderInfoLog(shader, logSize, std::addressof(chars), log.data());
            CHECK_ERROR("GetShaderLog", "glGetShaderInfoLog()");
        }
    }

    void GetProgramLog(GLuint program, std::string& log) {
        log.clear();
        GLint logSize{};
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, std::addressof(logSize));
        CHECK_ERROR("GetProgramLog", "glGetProgramiv(GL_INFO_LOG_LENGTH)");
        if (0 < logSize) {
            log.resize(logSize, '\0');
            GLsizei chars{};
            glGetProgramInfoLog(program, logSize, std::addressof(chars), log.data());
            CHECK_ERROR("GetProgramLog", "glGetProgramInfoLog()");
        }
    }
}

ShaderProgram::ShaderProgram(const std::string& vertex_shader, const std::string& fragment_shader) {
    glGetError();

    m_program_id = glCreateProgram();

    const char* strings[1] = { nullptr };

    if (!vertex_shader.empty()) {
        m_vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
        CHECK_ERROR("ShaderProgram::ShaderProgram", "glCreateShader(GL_VERTEX_SHADER)");

        strings[0] = vertex_shader.c_str();
        glShaderSource(m_vertex_shader_id, 1, strings, nullptr);
        CHECK_ERROR("ShaderProgram::ShaderProgram", "glShaderSource(vertex_shader)");

        glCompileShader(m_vertex_shader_id);
        CHECK_ERROR("ShaderProgram::ShaderProgram", "glCompileShader(m_vertex_shader_id)");

        GetShaderLog(m_vertex_shader_id, m_vertex_shader_log);

        glAttachShader(m_program_id, m_vertex_shader_id);
        CHECK_ERROR("ShaderProgram::ShaderProgram", "glAttachShader(m_vertex_shader_id)");
    }

    if (!fragment_shader.empty()) {
        m_fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
        CHECK_ERROR("ShaderProgram::ShaderProgram", "glCreateShader(GL_FRAGMENT_SHADER)");

        strings[0] = fragment_shader.c_str();
        glShaderSource(m_fragment_shader_id, 1, strings, nullptr);
        CHECK_ERROR("ShaderProgram::ShaderProgram", "glShaderSource(fragment_shader)");

        glCompileShader(m_fragment_shader_id);
        CHECK_ERROR("ShaderProgram::ShaderProgram", "glCompileShader(m_fragment_shader_id)");

        GetShaderLog(m_fragment_shader_id, m_fragment_shader_log);

        glAttachShader(m_program_id, m_fragment_shader_id);
        CHECK_ERROR("ShaderProgram::ShaderProgram", "glAttachShader(m_fragment_shader_id)");
    }

    GLint status{};
    glLinkProgram(m_program_id);
    CHECK_ERROR("ShaderProgram::ShaderProgram", "glLinkProgram()");
    glGetProgramiv(m_program_id, GL_LINK_STATUS, std::addressof(status));
    CHECK_ERROR("ShaderProgram::ShaderProgram", "glGetProgramiv(GL_LINK_STATUS)");
    m_link_succeeded = status;

    GetProgramLog(m_program_id, m_program_log);
}

std::unique_ptr<ShaderProgram> ShaderProgram::shaderProgramFactory(const std::string& vertex_shader,
                                                                   const std::string& fragment_shader)
{
    if (GetApp().GLVersion() >= 2.0f)
        return std::make_unique<ShaderProgram>(vertex_shader,fragment_shader);
    return nullptr;
}

ShaderProgram::~ShaderProgram() {
    glGetError();
    if (glIsShader(m_vertex_shader_id)) {
        GLint result = 0;
        glGetShaderiv(m_vertex_shader_id, GL_DELETE_STATUS, std::addressof(result));
        if (result) {
            glDeleteShader(m_vertex_shader_id);
            CHECK_ERROR("ShaderProgram::~ShaderProgram", "glDeleteShader(m_vertex_shader_id)");
        }
    }
    if (glIsShader(m_fragment_shader_id)) {
        GLint result = 0;
        glGetShaderiv(m_fragment_shader_id, GL_DELETE_STATUS, std::addressof(result));
        if (result) {
            glDeleteShader(m_fragment_shader_id);
            CHECK_ERROR("ShaderProgram::~ShaderProgram", "glDeleteShader(m_fragment_shader_id)");
        }
    }
    if (glIsProgram(m_program_id)) {
        GLint result = 0;
        glGetProgramiv(m_program_id, GL_DELETE_STATUS, std::addressof(result));
        if (result) {
            glDeleteProgram(m_program_id);
            CHECK_ERROR("ShaderProgram::~ShaderProgram", "glDeleteProgram()");
        }
    }
}

void ShaderProgram::Bind(const std::string& name, float f) {
    glGetError();
    GLint location = glGetUniformLocation(m_program_id, name.c_str());
    CHECK_ERROR("ShaderProgram::Bind", "glGetUniformLocation()");
    assert(location != -1 &&
           "Bind() : The named uniform variable does not exist.");
    glUniform1f(location, f);
    CHECK_ERROR("ShaderProgram::Bind", "glUniform1f()");
}

void ShaderProgram::Bind(const std::string& name, float f0, float f1) {
    glGetError();
    GLint location = glGetUniformLocation(m_program_id, name.c_str());
    CHECK_ERROR("ShaderProgram::Bind", "glGetUniformLocation()");
    assert(location != -1 &&
           "Bind() : The named uniform variable does not exist.");
    glUniform2f(location, f0, f1);
    CHECK_ERROR("ShaderProgram::Bind", "glUniform2f()");
}

void ShaderProgram::Bind(const std::string& name, float f0, float f1, float f2) {
    glGetError();
    GLint location = glGetUniformLocation(m_program_id, name.c_str());
    CHECK_ERROR("ShaderProgram::Bind", "glGetUniformLocation()");
    assert(location != -1 &&
           "Bind() : The named uniform variable does not exist.");
    glUniform3f(location, f0, f1, f2);
    CHECK_ERROR("ShaderProgram::Bind", "glUniform3f()");
}

void ShaderProgram::Bind(const std::string& name, float f0, float f1, float f2, float f3) {
    glGetError();
    GLint location = glGetUniformLocation(m_program_id, name.c_str());
    CHECK_ERROR("ShaderProgram::Bind", "glGetUniformLocation()");
    assert(location != -1 &&
           "Bind() : The named uniform variable does not exist.");
    glUniform4f(location, f0, f1, f2, f3);
    CHECK_ERROR("ShaderProgram::Bind", "glUniform4f()");
}

void ShaderProgram::Bind(const std::string& name, std::size_t element_size, const std::vector<float>& floats) {
    assert(1 <= element_size && element_size <= 4);
    assert((floats.size() % element_size) == 0);

    std::function<void (GLint, GLsizei, const GLfloat*)> functions[] =
        { glUniform1fv,
          glUniform2fv,
          glUniform3fv,
          glUniform4fv
        };

    glGetError();
    GLint location = glGetUniformLocation(m_program_id, name.c_str());
    CHECK_ERROR("ShaderProgram::Bind", "glGetUniformLocation()");
    assert(location != -1 &&
           "Bind() : The named uniform variable does not exist.");
    functions[element_size - 1](location, floats.size() / element_size, floats.data());
    CHECK_ERROR("ShaderProgram::Bind", "glUniformNfv()");
}

void ShaderProgram::Bind(const std::string& name, GLuint texture_id) {
    glGetError();
    GLint location = glGetUniformLocation(m_program_id, name.c_str());
    CHECK_ERROR("ShaderProgram::Bind", "glGetUniformLocation()");
    assert(location != -1 &&
           "Bind() : The named uniform sampler does not exist.");
    glUniform1i(location, texture_id);
    CHECK_ERROR("ShaderProgram::Bind", "glUniform1i()");
}

void ShaderProgram::BindInt(const std::string& name, int i) {
    glGetError();
    GLint location = glGetUniformLocation(m_program_id, name.c_str());
    CHECK_ERROR("ShaderProgram::BindInt", "glGetUniformLocation()");
    assert(location != -1 &&
           "BindInt() : The named uniform variable does not exist.");
    glUniform1i(location, i);
    CHECK_ERROR("ShaderProgram::BindInt", "glUniform1i()");
}

void ShaderProgram::BindInts(const std::string& name, int i0, int i1) {
    glGetError();
    GLint location = glGetUniformLocation(m_program_id, name.c_str());
    CHECK_ERROR("ShaderProgram::BindInts", "glGetUniformLocation()");
    assert(location != -1 &&
           "BindInts() : The named uniform variable does not exist.");
    glUniform2i(location, i0, i1);
    CHECK_ERROR("ShaderProgram::BindInts", "glUniform2i()");
}

void ShaderProgram::BindInts(const std::string& name, int i0, int i1, int i2) {
    glGetError();
    GLint location = glGetUniformLocation(m_program_id, name.c_str());
    CHECK_ERROR("ShaderProgram::BindInts", "glGetUniformLocation()");
    assert(location != -1 &&
           "BindInts() : The named uniform variable does not exist.");
    glUniform3i(location, i0, i1, i2);
    CHECK_ERROR("ShaderProgram::BindInts", "glUniform3i()");
}

void ShaderProgram::BindInts(const std::string& name, int i0, int i1, int i2, int i3) {
    glGetError();
    GLint location = glGetUniformLocation(m_program_id, name.c_str());
    CHECK_ERROR("ShaderProgram::BindInts", "glGetUniformLocation()");
    assert(location != -1 &&
           "BindInts() : The named uniform variable does not exist.");
    glUniform4i(location, i0, i1, i2, i3);
    CHECK_ERROR("ShaderProgram::BindInts", "glUniform4i()");
}

void ShaderProgram::BindInts(const std::string& name, std::size_t element_size, const std::vector<GLint>& ints) {
    assert(1 <= element_size && element_size <= 4);
    assert((ints.size() % element_size) == 0);

    std::function<void (GLint, GLsizei, const GLint*)> functions[] =
        { glUniform1iv,
          glUniform2iv,
          glUniform3iv,
          glUniform4iv
        };

    glGetError();
    GLint location = glGetUniformLocation(m_program_id, name.c_str());
    CHECK_ERROR("ShaderProgram::BindInts", "glGetUniformLocation()");
    assert(location != -1 &&
           "BindInts() : The named uniform variable does not exist.");
    functions[element_size - 1](location, ints.size() / element_size, ints.data());
    CHECK_ERROR("ShaderProgram::BindInts", "glUniformNiv()");
}

bool ShaderProgram::AllValuesBound() {
    bool retval = false;
    glGetError();
    glValidateProgram(m_program_id);
    CHECK_ERROR("ShaderProgram::AllValuesBound", "glValidateProgram()");
    GLint status;
    glGetProgramiv(m_program_id, GL_VALIDATE_STATUS, std::addressof(status));
    CHECK_ERROR("ShaderProgram::AllValuesBound", "glGetProgramiv(GL_VALIDATE_STATUS)");
    retval = status;
    GetProgramLog(m_program_id, m_program_validity_log);
    return retval;
}

void ShaderProgram::Use() const {
    glGetError();
    glUseProgram(m_program_id);
    CHECK_ERROR("ShaderProgram::Use", "glUseProgram()");
}

void ShaderProgram::stopUse() const
{ glUseProgram(0); }
