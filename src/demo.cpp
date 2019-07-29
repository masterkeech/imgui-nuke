static const char* const CLASS = "ImGuiDemo";
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


class ImGuiDemo : public NoIop, public ImGuiNuke
{
public:
    ImGuiDemo(Node* node) : NoIop(node) { }
    virtual void knobs(Knob_Callback);
    const char* Class() const { return CLASS; }
    const char* node_help() const { return HELP; }
    static const Iop::Description d;
    HandlesMode doAnyHandles(ViewerContext* ctx);
    void build_handles(ViewerContext* ctx);
    void draw_handle(ViewerContext* ctx);
    void _validate(bool);
};

void ImGuiDemo::knobs(Knob_Callback f)
{
    ImGuiKnobs(f);
}

static Iop* build(Node* node) { return new ImGuiDemo(node); }
const Iop::Description ImGuiDemo::d(CLASS, 0, build);

void ImGuiDemo::_validate(bool for_real)
{
    NoIop::_validate(for_real);
}


Op::HandlesMode ImGuiDemo::doAnyHandles(ViewerContext* ctx)
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
void ImGuiDemo::build_handles(ViewerContext* ctx)
{

    // Cause any input iop's to draw (you may want to skip this depending on what you do)
    build_input_handles(ctx);

    // Cause any knobs to draw (we don't have any so this makes no difference):
    build_knob_handles(ctx);

    // Don't draw anything unless viewer is in 2d mode:
    if (ctx->transform_mode() != VIEWER_2D) {
        return;
    }

    // make it call draw_handle():
    add_draw_handle(ctx);
}


// And then it will call this when stuff needs to be drawn:
void ImGuiDemo::draw_handle(ViewerContext* ctx)
{

    // You may want to do this if validate() calcuates anything you need to draw:
    // validate(false);

    // There are several "passes" and you should draw things during the correct
    // passes. There are a list of true/false tests on the ctx object to see if
    // something should be drawn during this pass.
    //
    // For 2D this will draw both a "shadow" line and a "real" line, but skip
    // all the other calls to draw_handle():
    if ( !ctx->draw_lines() ) {
        return;
    }

    NewFrame();

    ImGuiIO &io = GetImGuiIO();
    io.MousePos = ImVec2(ctx->mouse_x(), ctx->mouse_y());

    // Draw the demo window
    ImGui::ShowDemoWindow();

    // Rendering
    ImGui::Render();

    RenderDrawData(ImGui::GetDrawData());

    asapUpdate();
}
