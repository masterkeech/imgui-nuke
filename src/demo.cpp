static const char* const CLASS = "ImGuiDemo";
static const char* const HELP =
        "Displays the DearImGui Demo node.";

#include "DDImage/NoIop.h"
#include "DDImage/Row.h"
#include "DDImage/Knobs.h"
#include "DDImage/ViewerContext.h"
#include "DDImage/Knob.h"

#include "imgui.h"
#include "imgui_nuke.h"


using namespace DD::Image;


class ImGuiDemo : public NoIop, public ImGuiNuke
{
public:
    ImGuiDemo(Node* node) : NoIop(node), ImGuiNuke() { }
    ~ImGuiDemo()
    {
        // only destroy the context and cleanup the selection if we're the firstOp
        if (dynamic_cast<ImGuiDemo*>(firstOp()) == this)
        {
            Cleanup();
        }
    }

    bool BuildHandles(ViewerContext* ctx, Knob *knob)
    {
        // in those cases:
        // Don't draw anything unless viewer is in 2d mode:
        return ctx->transform_mode() == VIEWER_2D && knob->op()->panel_visible();
    }

    void Render(ViewerContext* ctx, Knob *knob)
    {
        // Draw the demo window
        ImGui::ShowDemoWindow();
    }

    void knobs(Knob_Callback f)
    {
        ImGuiKnobs(f);
    }

    const char* Class() const { return CLASS; }
    const char* node_help() const { return HELP; }
    static const Iop::Description d;
};

static Iop* build(Node* node) { return new ImGuiDemo(node); }
const Iop::Description ImGuiDemo::d(CLASS, 0, build);
