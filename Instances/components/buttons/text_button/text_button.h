#ifndef COMPONENTS_TEXT_BUTTON_H
#define COMPONENTS_TEXT_BUTTON_H

#include "native/components/base_button/base_button.h"
#include "native/components/buttons/text_button/text_button_assets.h"
#include "core/canvas/canvas_source.h"
#include "core/canvas/scoped_canvas.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    class TextButton : public BaseButton
    {
    private:
        absl::string_view kTextHeader;
        ComponentHandle<RenderComponent> render_component_;
        std::unique_ptr<CanvasSource> canvas_source_;
        ScopedCanvas::TextOptions kHeaderTextOptions_;
        void UpdateText();

    public:
        TextButton();
        void Setup(float2 size = {0.0f, 0.0f}, absl::string_view text = "Cancel");
        void SetNewColor(float4 color);
    };

}  // namespace

#endif // COMPONENTS_TEXT_BUTTON_H
