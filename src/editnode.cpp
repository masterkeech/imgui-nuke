static const char* const CLASS = "ImGuiEditNode";
static const char* const HELP =
        "Displays the DearImGui Demo node.";

#include "DDImage/NoIop.h"
#include "DDImage/Row.h"
#include "DDImage/Knobs.h"
#include "DDImage/gl.h"
#include "DDImage/ViewerContext.h"
#include "DDImage/DDMath.h"
#include "DDImage/Knob.h"

#include "imgui.h"
#include "imgui_nuke.h"


using namespace DD::Image;


class ImGuiEditNode : public NoIop, public ImGuiNuke
{
    void draw_window();
public:
    ImGuiEditNode(Node* node) : NoIop(node)
    {
        // set the glsl version to 120 as we have a crappy graphics card :(
        glsl_version_ = 120;
    }
    virtual void knobs(Knob_Callback);
    const char* Class() const { return CLASS; }
    const char* node_help() const { return HELP; }
    static const Iop::Description d;
    HandlesMode doAnyHandles(ViewerContext* ctx);
    bool handle(ViewerContext* ctx, int index);
    void build_handles(ViewerContext* ctx);
    void draw_handle(ViewerContext* ctx);
    void _validate(bool);
};

void ImGuiEditNode::knobs(Knob_Callback f)
{
    ImGuiKnobs(f);
}

static Iop* build(Node* node) { return new ImGuiEditNode(node); }
const Iop::Description ImGuiEditNode::d(CLASS, 0, build);

void ImGuiEditNode::_validate(bool for_real)
{
    NoIop::_validate(for_real);
}


Op::HandlesMode ImGuiEditNode::doAnyHandles(ViewerContext* ctx)
{
    Init(ctx->visibleViewportArea().w(), ctx->visibleViewportArea().h());

    if (ctx->transform_mode() == VIEWER_2D)
    {
        return eHandlesCooked;
    }

    return NoIop::doAnyHandles(ctx);
}


// This method is called to build a list of things to call to actually draw the viewer
// overlay. The reason for the two passes is so that 3D movements in the viewer can be
// faster by not having to call every node, but only the ones that asked for draw_handle
// to be called:
void ImGuiEditNode::build_handles(ViewerContext* ctx)
{

    // Cause any input iop's to draw (you may want to skip this depending on what you do)
    build_input_handles(ctx);

    // Cause any knobs to draw (we don't have any so this makes no difference):
    build_knob_handles(ctx);

    // Don't draw anything unless viewer is in 2d mode:
    if (ctx->transform_mode() != VIEWER_2D)
        return;

    // make it call draw_handle():
    add_draw_handle(ctx);
}


// And then it will call this when stuff needs to be drawn:
void ImGuiEditNode::draw_handle(ViewerContext* ctx)
{
    // You may want to do this if validate() calcuates anything you need to draw:
    // validate(false);

    // There are several "passes" and you should draw things during the correct
    // passes. There are a list of true/false tests on the ctx object to see if
    // something should be drawn during this pass.
    //
    // For 2D this will draw both a "shadow" line and a "real" line, but skip
    // all the other calls to draw_handle():
    if ( !ctx->draw_lines() )
        return;

    NewFrame();

    ImGuiIO &io = GetImGuiIO();
    io.MousePos = ImVec2(ctx->mouse_x(), ctx->mouse_y());

    // Draw the window
    draw_window();

    // Rendering
    ImGui::Render();
    int display_w, display_h;

    RenderDrawData(ImGui::GetDrawData());

    asapUpdate();
}

void ImGuiEditNode::draw_window()
{
    // Draw the gui
    ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("ImGui Edit Node"))
    {
        // Early out if the window is collapsed, as an optimization.
        ImGui::End();
        return;
    }

    if (!input(0) || !strcmp(input0().Class(), "Black")) {
        ImGui::Text("Please attach node to edit.");
        ImGui::Spacing();
        ImGui::End();
        return;
    }

    std::cerr << "Node: " << input0().node_name() << " Class: " << input0().Class() << std::endl;

    unsigned int i = 0;
    while (Knob *k = input0().knob(i++))
    {
        // if we've hit the name then we're in the Node's default knobs
        if (k->name() == "name")
            break;

        switch (k->ClassID())
        {
            case BOOL_KNOB:
                break;
            case INT_KNOB:
                break;
            case DOUBLE_KNOB:
                break;
            case FLOAT_KNOB: {
                float *value = (float *)field(k->name().c_str());
                if (value) {
                    std::cerr << "Name: " << k->name() << " ID: " << k->ClassID() << " value: " << *value << std::endl;
                    ImGui::SliderFloat(k->label().c_str(), value, 0, 100);
                }
                break;
            }
            case COLOR_KNOB:
                break;
            case ACOLOR_KNOB:
                break;
            default:
                break;
        }
    }
    ImGui::End();
}
