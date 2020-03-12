#ifndef IMGUI_NUKE_HEADER
#define IMGUI_NUKE_HEADER

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "imgui.h"

#include "DDImage/gl.h"
#include "DDImage/Knob.h"
#include "DDImage/Knobs.h"

#ifndef DEBUG
#define DEBUG 0
#endif

using namespace DD::Image;

template<class T>
class ImGuiKnob : public Knob
{
    T* theOp;
    const char* Class() const { return "ImGuiKnob"; }
public:

    // This is what Nuke will call once the below stuff is executed:
    static bool handle_cb(ViewerContext* ctx, Knob* knob, int index)
    {
        if (DEBUG)
        {
            printf("%s ", knob->op()->node_name().c_str());
        }
        return ((ImGuiKnob*)knob)->theOp->Handle(ctx, index);
    }

    ImGuiKnob(Knob_Closure* kc, T* t, const char* n) : Knob(kc, n)
    {
        theOp = t;
    }

    // Nuke calls this to draw the handle, this then calls make_handle
    // which tells Nuke to call the above function when the mouse does
    // something...
    void draw_handle(ViewerContext* ctx)
    {
        // There are several "passes" and you should draw things during the correct
        // passes. There are a list of true/false tests on the ctx object to see if
        // something should be drawn during this pass.
        //
        // For 2D this will draw both a "shadow" line and a "real" line, but skip
        // all the other calls to draw_handle():
        if (!ctx->draw_lines())
        {
            return;
        }

        // create the new frame for imgui
        theOp->NewFrame();

        // update the the mouse cursor position for the imgui io
        ImGuiIO &io = theOp->GetImGuiIO();
        io.MousePos = ImVec2(ctx->mouse_x(), ctx->mouse_y());

        // Rendering the custom imgui setup
        theOp->Render(ctx, (Knob*)this);

        // Rendering
        ImGui::Render();

        theOp->RenderDrawData(ImGui::GetDrawData());

        // draw the selection area
        if (ctx->event() == DRAW_OPAQUE
            || ctx->event() == PUSH // true for clicking hit-detection
            || ctx->event() == DRAG // true for selection box hit-detection
                ) {
            // Make clicks anywhere in the viewer call handle() with index = 0.
            // This takes the lowest precedence over, so above will be detected
            // first.
            begin_handle(Knob::ANYWHERE, ctx, handle_cb, 0 /*index*/, 0, 0, 0 /*xyz*/);
            end_handle(ctx);
        }

        asapUpdate();
    }

    // And you need to implement this just to make it call draw_handle:
    bool build_handle(ViewerContext* ctx)
    {
#ifdef __APPLE__
        theOp->Init(ctx->visibleViewportArea().w(), ctx->visibleViewportArea().h());
#else
        theOp->Init(ctx->viewport().w(), ctx->viewport().h());
#endif
        return theOp->BuildHandles(ctx, (Knob*)this);
    }
};


class ImGuiNuke
{
protected:
    // OpenGL Data
    GLuint       font_texture_;
    GLuint       shader_handle_, vert_handle_, frag_handle_;
    int          attrib_location_tex_, attrib_location_proj_matrix_;
    int          attrib_location_position_, attrib_location_uv_, attrib_location_color_;
    unsigned int vbo_handle_, elements_handle_;
    ImGuiContext* context_;


    // If you get an error please report on github. You may try different GL context version or GLSL version. See GL<>GLSL version table at the top of this file.
    static bool CheckShader(GLuint handle, const char* desc)
    {
        GLint status = 0, log_length = 0;
        glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
        if ((GLboolean)status == GL_FALSE) {
            fprintf(stderr, "ERROR: CreateDeviceObjects: failed to compile %s!\n", desc);
        }
        if (log_length > 0)
        {
            ImVector<char> buf;
            buf.resize((int)(log_length + 1));
            glGetShaderInfoLog(handle, log_length, NULL, (GLchar*)buf.begin());
            fprintf(stderr, "%s\n", buf.begin());
        }
        return (GLboolean)status == GL_TRUE;
    }

    // If you get an error please report on GitHub. You may try different GL context version or GLSL version.
    static bool CheckProgram(GLuint handle, const char* desc)
    {
        GLint status = 0, log_length = 0;
        glGetProgramiv(handle, GL_LINK_STATUS, &status);
        glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
        if ((GLboolean)status == GL_FALSE) {
            fprintf(stderr, "ERROR: CreateDeviceObjects: failed to link %s! (with GLSL 410)\n", desc);
        }
        if (log_length > 0)
        {
            ImVector<char> buf;
            buf.resize((int)(log_length + 1));
            glGetProgramInfoLog(handle, log_length, NULL, (GLchar*)buf.begin());
            fprintf(stderr, "%s\n", buf.begin());
        }
        return (GLboolean)status == GL_TRUE;
    }

    bool CreateFontsTexture()
    {
        if (DEBUG) {
            std::cerr << "CreateFontsTexture start" << std::endl;
        }
        // Build texture atlas
        ImGuiIO& io = GetImGuiIO();
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

        // Upload texture to graphics system
        GLint last_texture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
        glGenTextures(1, &font_texture_);
        glBindTexture(GL_TEXTURE_2D, font_texture_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        // Store our identifier
        io.Fonts->TexID = (ImTextureID)(intptr_t)font_texture_;

        // Restore state
        glBindTexture(GL_TEXTURE_2D, last_texture);

        if (DEBUG) {
            std::cerr << "CreateFontsTexture end" << std::endl;
        }

        return true;
    }

    void DestroyFontsTexture()
    {
        if (font_texture_)
        {
            if (DEBUG) {
                std::cerr << "DestroyFontsTexture" << std::endl;
            }
            ImGuiIO& io = ImGui::GetIO();
            glDeleteTextures(1, &font_texture_);
            io.Fonts->TexID = 0;
            font_texture_ = 0;
        }
    }

    void DestroyDeviceObjects()
    {
        if (DEBUG) {
            std::cerr << "DestroyDeviceObjects" << std::endl;
        }
        if (vbo_handle_) glDeleteBuffers(1, &vbo_handle_);
        if (elements_handle_) glDeleteBuffers(1, &elements_handle_);
        vbo_handle_ = elements_handle_ = 0;

        if (shader_handle_ && vert_handle_) glDetachShader(shader_handle_, vert_handle_);
        if (vert_handle_) glDeleteShader(vert_handle_);
        vert_handle_ = 0;

        if (shader_handle_ && frag_handle_) glDetachShader(shader_handle_, frag_handle_);
        if (frag_handle_) glDeleteShader(frag_handle_);
        frag_handle_ = 0;

        if (shader_handle_) glDeleteProgram(shader_handle_);
        shader_handle_ = 0;

        DestroyFontsTexture();
    }

    bool CreateDeviceObjects()
    {
        if (DEBUG) {
            std::cerr << "CreateDeviceObjects start" << std::endl;
        }
        // Backup GL state
        GLint last_texture, last_array_buffer, last_vertex_array;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

        //
        int glsl_version;
        std::string gls_version_string;
        // Parse GLSL version string
        {
            std::stringstream gl_version_stream;
            gl_version_stream << glGetString(GL_SHADING_LANGUAGE_VERSION);
            glsl_version = int(std::stof(gl_version_stream.str()) * 100.0f);
            std::stringstream gl_string_stream;
            gl_string_stream << "#version " << glsl_version << std::endl;
            gls_version_string = gl_string_stream.str();
        }

        const GLchar* vertex_shader_glsl_120 =
                "uniform mat4 ProjMtx;\n"
                "attribute vec2 Position;\n"
                "attribute vec2 UV;\n"
                "attribute vec4 Color;\n"
                "varying vec2 Frag_UV;\n"
                "varying vec4 Frag_Color;\n"
                "void main()\n"
                "{\n"
                "    Frag_UV = UV;\n"
                "    Frag_Color = Color;\n"
                "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
                "}\n";

        const GLchar* vertex_shader_glsl_130 =
                "uniform mat4 ProjMtx;\n"
                "in vec2 Position;\n"
                "in vec2 UV;\n"
                "in vec4 Color;\n"
                "out vec2 Frag_UV;\n"
                "out vec4 Frag_Color;\n"
                "void main()\n"
                "{\n"
                "    Frag_UV = UV;\n"
                "    Frag_Color = Color;\n"
                "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
                "}\n";

        const GLchar* vertex_shader_glsl_300_es =
                "precision mediump float;\n"
                "layout (location = 0) in vec2 Position;\n"
                "layout (location = 1) in vec2 UV;\n"
                "layout (location = 2) in vec4 Color;\n"
                "uniform mat4 ProjMtx;\n"
                "out vec2 Frag_UV;\n"
                "out vec4 Frag_Color;\n"
                "void main()\n"
                "{\n"
                "    Frag_UV = UV;\n"
                "    Frag_Color = Color;\n"
                "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
                "}\n";

        const GLchar* vertex_shader_glsl_410_core =
                "layout (location = 0) in vec2 Position;\n"
                "layout (location = 1) in vec2 UV;\n"
                "layout (location = 2) in vec4 Color;\n"
                "uniform mat4 ProjMtx;\n"
                "out vec2 Frag_UV;\n"
                "out vec4 Frag_Color;\n"
                "void main()\n"
                "{\n"
                "    Frag_UV = UV;\n"
                "    Frag_Color = Color;\n"
                "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
                "}\n";

        const GLchar* fragment_shader_glsl_120 =
                "#ifdef GL_ES\n"
                "    precision mediump float;\n"
                "#endif\n"
                "uniform sampler2D Texture;\n"
                "varying vec2 Frag_UV;\n"
                "varying vec4 Frag_Color;\n"
                "void main()\n"
                "{\n"
                "    gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV.st);\n"
                "}\n";

        const GLchar* fragment_shader_glsl_130 =
                "uniform sampler2D Texture;\n"
                "in vec2 Frag_UV;\n"
                "in vec4 Frag_Color;\n"
                "out vec4 Out_Color;\n"
                "void main()\n"
                "{\n"
                "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
                "}\n";

        const GLchar* fragment_shader_glsl_300_es =
                "precision mediump float;\n"
                "uniform sampler2D Texture;\n"
                "in vec2 Frag_UV;\n"
                "in vec4 Frag_Color;\n"
                "layout (location = 0) out vec4 Out_Color;\n"
                "void main()\n"
                "{\n"
                "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
                "}\n";

        const GLchar* fragment_shader_glsl_410_core =
                "in vec2 Frag_UV;\n"
                "in vec4 Frag_Color;\n"
                "uniform sampler2D Texture;\n"
                "layout (location = 0) out vec4 Out_Color;\n"
                "void main()\n"
                "{\n"
                "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
                "}\n";

        // Select shaders matching our GLSL versions
        const GLchar* vertex_shader = NULL;
        const GLchar* fragment_shader = NULL;
        if (glsl_version < 130)
        {
            vertex_shader = vertex_shader_glsl_120;
            fragment_shader = fragment_shader_glsl_120;
        }
        else if (glsl_version >= 410)
        {
            vertex_shader = vertex_shader_glsl_410_core;
            fragment_shader = fragment_shader_glsl_410_core;
        }
        else if (glsl_version == 300)
        {
            vertex_shader = vertex_shader_glsl_300_es;
            fragment_shader = fragment_shader_glsl_300_es;
        }
        else
        {
            vertex_shader = vertex_shader_glsl_130;
            fragment_shader = fragment_shader_glsl_130;
        }

        // Create shaders
        const GLchar* vertex_shader_with_version[2] = { gls_version_string.c_str(), vertex_shader };
        vert_handle_ = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vert_handle_, 2, vertex_shader_with_version, NULL);
        glCompileShader(vert_handle_);
        CheckShader(vert_handle_, "vertex shader");

        const GLchar* fragment_shader_with_version[2] = { gls_version_string.c_str(), fragment_shader };
        frag_handle_ = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(frag_handle_, 2, fragment_shader_with_version, NULL);
        glCompileShader(frag_handle_);
        CheckShader(frag_handle_, "fragment shader");

        shader_handle_ = glCreateProgram();
        glAttachShader(shader_handle_, vert_handle_);
        glAttachShader(shader_handle_, frag_handle_);
        glLinkProgram(shader_handle_);
        CheckProgram(shader_handle_, "shader program");

        attrib_location_tex_ = glGetUniformLocation(shader_handle_, "Texture");
        attrib_location_proj_matrix_ = glGetUniformLocation(shader_handle_, "ProjMtx");
        attrib_location_position_ = glGetAttribLocation(shader_handle_, "Position");
        attrib_location_uv_ = glGetAttribLocation(shader_handle_, "UV");
        attrib_location_color_ = glGetAttribLocation(shader_handle_, "Color");

        // Create buffers
        glGenBuffers(1, &vbo_handle_);
        glGenBuffers(1, &elements_handle_);

        CreateFontsTexture();

        // Restore modified GL state
        glBindTexture(GL_TEXTURE_2D, last_texture);
        glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
        glBindVertexArray(last_vertex_array);

        if (DEBUG) {
            std::cerr << "CreateDeviceObjects end" << std::endl;
        }

        return true;
    }

public:

    ImGuiNuke() : font_texture_(0), shader_handle_(0), vert_handle_(0), frag_handle_(0),
                  attrib_location_tex_(0), attrib_location_proj_matrix_(0), attrib_location_position_(0),
                  attrib_location_uv_(0), attrib_location_color_(0), vbo_handle_(0), elements_handle_(0), context_(nullptr)
    {}

    void Cleanup()
    {
        if (context_) {
            if (DEBUG) {
                std::cerr << "cleaning up begin" << std::endl;
            }
            ImGui::SetCurrentContext(context_);
            DestroyDeviceObjects();
            ImGui::DestroyContext(context_);
            if (DEBUG) {
                std::cerr << "cleaning up end" << std::endl;
            }
        }
    }

    bool Handle(ViewerContext* ctx, int index)
    {
        ImGuiIO &io = GetImGuiIO();
        if (DEBUG) {
            printf("Index %d: ", index);
        }
        // swap middle mouse button and right mouse click
        auto get_button = [](int b) { return b == 1 || b > 3 ? b - 1 : (b - 1) % 2 + 1; };
        switch (ctx->event()) {
            case PUSH:
            {
                if (DEBUG) {
                    printf("PUSH");
                }
                io.MouseDown[get_button(ctx->button())] = true;
                break;
            }
            case DRAG:
                if (DEBUG) {
                    printf("DRAG");
                }
                break;
            case RELEASE:
            {
                if (DEBUG) {
                    printf("RELEASE");
                }
                io.MouseDown[get_button(ctx->button())] = false;
                break;
            }
            case MOVE:
                if (DEBUG) {
                    printf("MOVE");
                }
                break;
                // MOVE will only work if you use ANYWHERE_MOUSEMOVES instead of
                // ANYWHERE below.
            default:
                if (DEBUG) {
                    printf("event()==%d", ctx->event());
                }
                break;
        }
        io.KeyCtrl = ctx->event() != RELEASE && (ctx->state() & CTRL) != 0;
        io.KeyShift = ctx->event() != RELEASE && (ctx->state() & SHIFT) != 0;
        io.KeyAlt = ctx->event() != RELEASE && (ctx->state() & ALT) != 0;
        io.MousePos = ImVec2(ctx->mouse_x(), ctx->mouse_y());
        if (DEBUG) {
            printf(" xyz=%g,%g,%g", ctx->x(), ctx->y(), ctx->z());
            printf(" mousexy=%d,%d", ctx->mouse_x(), ctx->mouse_y());
            printf(" button=%d state=%d", ctx->button(), ctx->state());
            printf(" ctrl=%d shift=%d alt=%d", io.KeyCtrl, io.KeyShift, io.KeyAlt);
            printf("\n");
        }
        return true; // true means we are interested in the event
    }

    void Init(unsigned int width, unsigned int height)
    {
        if (context_ == nullptr)
        {
            context_ = ImGui::CreateContext();
            if (DEBUG) {
                std::cerr << "creating imgui context: " << context_ << std::endl;
            }
            ImGui::StyleColorsDark();
        }
        if (context_)
        {
            ImGuiIO &io = GetImGuiIO();
            io.DisplaySize = ImVec2(width, height);

        }
    }

    void NewFrame()
    {
        ImGui::SetCurrentContext(context_);
        if (!font_texture_)
        {
            CreateDeviceObjects();
        }
        ImGui::NewFrame();
    }

    ImGuiIO& GetImGuiIO()
    {
        ImGui::SetCurrentContext(context_);
        ImGuiIO& io = ImGui::GetIO();
        return io;
    }

    void ImGuiKnobs(Knob_Callback f)
    {
        CustomKnob1(ImGuiKnob<ImGuiNuke>, f, this, "kludge");
    }

    // OpenGL3 Render function.
    // (this used to be set in io.RenderDrawListsFn and called by ImGui::Render(), but you can now call this directly from your main loop)
    // Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly, in order to be able to run within any OpenGL engine that doesn't do so.
    void RenderDrawData(ImDrawData* draw_data)
    {
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
        int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
        if (fb_width <= 0 || fb_height <= 0)
            return;

        // Backup GL state
        GLenum last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&last_active_texture);
        glActiveTexture(GL_TEXTURE0);
        GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
        GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
#ifdef GL_SAMPLER_BINDING
        GLint last_sampler; glGetIntegerv(GL_SAMPLER_BINDING, &last_sampler);
#endif
        GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
        GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
#ifdef GL_POLYGON_MODE
        GLint last_polygon_mode[2]; glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
#endif
        GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
        GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
        GLenum last_blend_src_rgb; glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&last_blend_src_rgb);
        GLenum last_blend_dst_rgb; glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&last_blend_dst_rgb);
        GLenum last_blend_src_alpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&last_blend_src_alpha);
        GLenum last_blend_dst_alpha; glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&last_blend_dst_alpha);
        GLenum last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&last_blend_equation_rgb);
        GLenum last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&last_blend_equation_alpha);
        GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
        GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
        GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
        GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
        bool clip_origin_lower_left = true;
#ifdef GL_CLIP_ORIGIN
        GLenum last_clip_origin = 0; glGetIntegerv(GL_CLIP_ORIGIN, (GLint*)&last_clip_origin); // Support for GL 4.5's glClipControl(GL_UPPER_LEFT)
    if (last_clip_origin == GL_UPPER_LEFT)
        clip_origin_lower_left = false;
#endif

        // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_SCISSOR_TEST);
#ifdef GL_POLYGON_MODE
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif

        // Setup viewport, orthographic projection matrix
        // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayMin is typically (0,0) for single viewport apps.
        glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
        float L = draw_data->DisplayPos.x;
        float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
        float T = draw_data->DisplayPos.y;
        float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
        const float ortho_projection[4][4] =
                {
                        { 2.0f/(R-L),   0.0f,         0.0f,   0.0f },
                        { 0.0f,         2.0f/(T-B),   0.0f,   0.0f },
                        { 0.0f,         0.0f,        -1.0f,   0.0f },
                        { (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f },
                };
        glUseProgram(shader_handle_);
        glUniform1i(attrib_location_tex_, 0);
        glUniformMatrix4fv(attrib_location_proj_matrix_, 1, GL_FALSE, &ortho_projection[0][0]);
#ifdef GL_SAMPLER_BINDING
        glBindSampler(0, 0); // We use combined texture/sampler state. Applications using GL 3.3 may set that otherwise.
#endif
        // Recreate the VAO every time
        // (This is to easily allow multiple GL contexts. VAO are not shared among GL contexts, and we don't track creation/deletion of windows so we don't have an obvious key to use to cache them.)
        GLuint vao_handle = 0;
        glGenVertexArrays(1, &vao_handle);
        glBindVertexArray(vao_handle);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_handle_);
        glEnableVertexAttribArray(attrib_location_position_);
        glEnableVertexAttribArray(attrib_location_uv_);
        glEnableVertexAttribArray(attrib_location_color_);
        glVertexAttribPointer(attrib_location_position_, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
        glVertexAttribPointer(attrib_location_uv_, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
        glVertexAttribPointer(attrib_location_color_, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col));

        // Will project scissor/clipping rectangles into framebuffer space
        ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
        ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

        // Render command lists
        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            size_t idx_buffer_offset = 0;

            glBindBuffer(GL_ARRAY_BUFFER, vbo_handle_);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elements_handle_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback)
                {
                    // User callback (registered via ImDrawList::AddCallback)
                    pcmd->UserCallback(cmd_list, pcmd);
                }
                else
                {
                    // Project scissor/clipping rectangles into framebuffer space
                    ImVec4 clip_rect;
                    clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                    clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                    clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                    clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                    if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                    {
                        // Apply scissor/clipping rectangle
                        if (clip_origin_lower_left) {
                            glScissor((int) clip_rect.x, (int) (fb_height - clip_rect.w), (int) (clip_rect.z - clip_rect.x),
                                      (int) (clip_rect.w - clip_rect.y));
                        } else {
                            glScissor((int) clip_rect.x, (int) clip_rect.y, (int) clip_rect.z, (int) clip_rect.w); // Support for GL 4.5's glClipControl(GL_UPPER_LEFT)
                        }

                        // Bind texture, Draw
                        glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);
                        glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (void*)idx_buffer_offset);
                    }
                }
                idx_buffer_offset += pcmd->ElemCount * sizeof(ImDrawIdx);
            }
        }
        glDeleteVertexArrays(1, &vao_handle);

        // Restore modified GL state
        glUseProgram(last_program);
        glBindTexture(GL_TEXTURE_2D, last_texture);
#ifdef GL_SAMPLER_BINDING
        glBindSampler(0, last_sampler);
#endif
        glActiveTexture(last_active_texture);
        glBindVertexArray(last_vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
        glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
        glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
        if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
        if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
        if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
#ifdef GL_POLYGON_MODE
        glPolygonMode(GL_FRONT_AND_BACK, (GLenum)last_polygon_mode[0]);
#endif
        glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
        glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
    }

    // used to render your custom imgui ui
    virtual void Render(ViewerContext* ctx, Knob *knob) = 0;

    // determine when to build the handles, eg. 3d vs. 2d
    virtual bool BuildHandles(ViewerContext* ctx, Knob *knob) = 0;
};

#endif