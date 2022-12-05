#include <sb7.h>

#include <shader.h>
#include <object.h>
#include <vmath.h>

class reversedz_app : public sb7::application
{

public:

    reversedz_app() 
        : reverse(false),
          show_depth(false)
    {
    }

    void startup();
    void render(double currentTime);
    void onKey(int key, int action);
    void shutdown();

private:

    void init()
    {
        static const char title[] = "OpenGL Reversed Z";

        sb7::application::init();

        info.flags.debug = 1;

        memcpy(info.title, title, sizeof(title));
    }

    void load_shaders();

    bool reverse;
    bool show_depth;

    sb7::object dragon_obj;
    sb7::object cube_obj;

    GLuint render_prg;

    GLuint quad_vao;
    GLuint blit_prg;

    GLuint fbo;
    GLuint fbo_textures[2];

    struct
    {
        GLint           mv_matrix;
        GLint           proj_matrix;
    } uniforms;
};

void reversedz_app::startup()
{
    bool reversedZ = sb7IsExtensionSupported("GL_ARB_clip_control");
    sb7::WriteLog("Is GL_ARB_clip_control supported: %s\n", reversedZ ? "true" : "false");

    load_shaders();

    const int width = info.windowWidth;
    const int height = info.windowHeight;

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenTextures(2, fbo_textures);

    glBindTexture(GL_TEXTURE_2D, fbo_textures[0]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB16F, width, height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, fbo_textures[1]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, width, height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, fbo_textures[0], 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, fbo_textures[1], 0);

    static const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, draw_buffers);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glEnable(GL_CULL_FACE);

    glEnable(GL_DEPTH_TEST); 

    glGenVertexArrays(1, &quad_vao);
    glBindVertexArray(quad_vao);

    dragon_obj.load("media/objects/dragon.sbm");
    cube_obj.load("media/objects/cube.sbm");
}

void reversedz_app::shutdown()
{
    glDeleteVertexArrays(1, &quad_vao);
    glDeleteProgram(render_prg);
    glDeleteProgram(blit_prg);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(2, fbo_textures);
}

void reversedz_app::render(double currentTime)
{
    static const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    static const GLfloat one = 1.0f;
    static const GLfloat zero = 0.0f;

    float f = 1.0f;

    glViewport(0, 0, info.windowWidth, info.windowHeight);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);

    glEnable(GL_DEPTH_TEST);

    glDepthFunc(reverse ? GL_GREATER : GL_LESS);

    glClearBufferfv(GL_COLOR, 0, black);
    glClearBufferfv(GL_DEPTH, 0, reverse ? &zero : &one);
    
    glUseProgram(render_prg);

    const vmath::mat4 lookat_matrix = vmath::lookat(vmath::vec3(0.0f, 3.0f, 15.0f),
                                                    vmath::vec3(0.0f, 0.0f, 0.0f),
                                                    vmath::vec3(0.0f, 1.0f, 0.0f));

    vmath::mat4 proj_matrix = vmath::perspective_z01(50.0f,
        (float)info.windowWidth / (float)info.windowHeight, 5.0f, 1000.0f);

    if (reverse)
    {
        f = 0.5f;
        proj_matrix = vmath::perspective_z10(50.0f,
            (float)info.windowWidth / (float)info.windowHeight, 5.0f, 1000.0f);
    }

    glUniformMatrix4fv(uniforms.proj_matrix, 1, GL_FALSE, proj_matrix);

    vmath::mat4 model_matrix = vmath::translate(0.0f, -5.0f, 0.0f) *
        vmath::rotate(f * 45.0f, 0.0f, 1.0f, 0.0f) *
        vmath::mat4::identity();

    glUniformMatrix4fv(uniforms.mv_matrix, 1, GL_FALSE, lookat_matrix * model_matrix);

    dragon_obj.render();

    model_matrix = vmath::translate(0.0f, -4.5f, 0.0f) *
        vmath::rotate(5.0f, 0.0f, 1.0f, 0.0f) *
        vmath::scale(4000.0f, 0.1f, 4000.0f) *
        vmath::mat4::identity();
    glUniformMatrix4fv(uniforms.mv_matrix, 1, GL_FALSE, lookat_matrix * model_matrix);

    cube_obj.render();
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClearBufferfv(GL_COLOR, 0, black);
    glClearBufferfv(GL_DEPTH, 0, reverse ? &zero : &one);

    glUseProgram(blit_prg);

    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, show_depth ? fbo_textures[1] : fbo_textures[0]);
    
    glBindVertexArray(quad_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void reversedz_app::load_shaders()
{
    GLuint shaders[2];

    shaders[0] = sb7::shader::load("media/shaders/render.vs.glsl", GL_VERTEX_SHADER);
    shaders[1] = sb7::shader::load("media/shaders/render.fs.glsl", GL_FRAGMENT_SHADER);

    if (render_prg)
        glDeleteProgram(render_prg);

    render_prg = sb7::program::link_from_shaders(shaders, 2, true);

    uniforms.mv_matrix = glGetUniformLocation(render_prg, "mv_matrix");
    uniforms.proj_matrix = glGetUniformLocation(render_prg, "proj_matrix");
    
    shaders[0] = sb7::shader::load("media/shaders/blit.vs.glsl", GL_VERTEX_SHADER);
    shaders[1] = sb7::shader::load("media/shaders/blit.fs.glsl", GL_FRAGMENT_SHADER);

    if (blit_prg)
        glDeleteProgram(blit_prg);

    blit_prg = sb7::program::link_from_shaders(shaders, 2, true);    
}

void reversedz_app::onKey(int key, int action)
{
    if (action)
    {
        switch (key)
        {
            case 'R':
                reverse = !reverse;
                break;
            case 'T':
                show_depth = !show_depth;
                break;
            case 'L':
                load_shaders();
                break;
        }
    }
}

DECLARE_MAIN(reversedz_app)
