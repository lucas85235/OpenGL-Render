#ifndef COMPONENTS_BASE_BUTTON_H
#define COMPONENTS_BASE_BUTTON_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "native/components/interaction/interaction.h"

using namespace imp;
namespace ix::samsung::homecomponents
{
    struct ButtonProperties{
        float2 canvas_size = {0.0, 0.0};
        float2 button_size = {0.0, 0.0};
        float2 corner_radius = {0.0, 0.0};
        float3 color_rect = {0.0, 0.0, 0.0};
    };

    class BaseButton : public Component
    {
    private:
        NodeHandle backplate_node_;
        NodeHandle frontplate_node_;
        NodeHandle highlight_node_;
        ButtonProperties properties_;
        ComponentHandle<Interactable> interaction_;
        Material* button_material_;
        Material* highlight_material_;
        float duration_ = 0.3f;
        float elapsed_ = 0.0f;
        bool is_in_lerp_ = false;
        float3 initial_position_ = {0.0f, 0.0f, 0.4};
        float3 final_position_ = {0.0f, 0.0f, 0.01};

    public:
        void Setup();
        void Setup(ButtonProperties properties);
        void Update(const FrameTime &frame_time);
        void ButtonClicked();
        void ButtonHovered();
        void ButtonNormal();
        NodeHandle GetFrontPlateNode();

    protected:
        virtual void ChildUpdate();
    };

}  // namespace

#endif // COMPONENTS_BASE_BUTTON_H