#pragma once

#include "core/ncsb/component.h"
#include "imp.h"
#include "native/components/interaction/interactor_factory.h"
#include "native/components/interaction/interactable.h"
#include "native/components/UI/carousel/carousel_assets.h"
#include "native/home_components_utils/common/math/math.h"

#if IMP_PLATFORM(ANDROID)
#include "core/actions/controller_events.h"
#include "core/view/platforms/xr_android/xr_action_controller.h"
#include "core/view/platforms/xr_android/xr_opengl_platform.h"
#include "core/xr/openxr_events.h"
#include "core/actions/input_action_event.h"
#include "core/actions/controller_events.h"
#include "core/actions/action_config.h"
#include "core/actions/controller_input_handler.h"
#endif

#if IMP_RUNTIME(DEV)
#define DEBUG debug_draw
#define DEBUG_GLOBAL debug_draw::Global()
#define DEBUG_COLOR debug_draw::GetColor
#define BLUE debug_draw::DebugColor::kBlue
#define RED debug_draw::DebugColor::kRed
#define GREEN debug_draw::DebugColor::kGreen
#endif

using namespace imp;
using namespace std;

#define DEBUG_CAROUSEL false

namespace ix::samsung::homecomponents
{
    struct OnElementSelected : Event
    {
        int element_index;
        NodeHandle element;
        NodeHandle last_selected_element;
    };

    struct OnAnimationStarted : Event
    {
        NodeHandle last_selected_element;
    };

    struct OnElementClicked : Event
    {
        int element_index;
    };

    class Carousel : public Component
    {
    private:
        void PrintDebugMessage();
        NodeHandle CreateShapeNode(MeshPtr shape_mesh, float4 color);
        NodeHandle AddBoxModel(const string& name, float4 color);
        ComponentHandle<Interactable> interactable_component_;
        OnElementSelected on_element_selected_ = OnElementSelected();
        OnAnimationStarted on_animation_started_ = OnAnimationStarted();
        NodeHandle interactor_node_, carousel_node_, carousel_internal_holder_node_;
        NodeHandle last_selected_carousel_element_;
        vector<NodeHandle> carousel_elements_;
        Box carousel_collider_;
        State state_ = NORMAL;
        // TODO(rafael): this variable will be used in the future determine the carousel spin direction
        float3 scroll_orientation_ = float3(1.0, 0.0, 0.0);
        // just to keep tracking about the last pointer position to calculate the delta
        float3 last_pointer_position_ = 0.0;
        // the last position that a pointer up event occurs
        float3 last_click_position_ = 0.0;
        float3 last_hover_position_ = 0.0;
        // TODO(rafael): this will be passed to the material in the future to discard the correct pixels
        float3 interaction_viewport_size_ = float3(0.6, 0.4, 0.1);
        // the amount of objects in the carousel
        int objects_amount_ = 0;
        // the index of the item that is centralized
        int selected_object_index_ = 0;
        // this is the total size of a carousel cell
        float carousel_cell_size_ = 0.3;
        // the space between the elements
        float elements_spacing_ = 0.12;
        // this is the float index that controls how much the carousel spins
        float offset_index_ = 0.0;
        // the amount of how much will be multiplied by the pointer_delta_position_
        float drag_sensitivity_ = 5.0;
        // the carousel total radius
        float radius_ = 1.5f;
        // this is the max and min limit of how much the pointer_delta_position_ can grow
        float max_movement_delta_ = 0.03;
        // this is the amount of time that a snap to the correct position can take
        float snap_interpolation_seconds_ = 0.3;
        // this is the difference between the current and the last frame pointer location
        float pointer_delta_position_ = 0.0;
        // This is the amount of negative acceleration that is applied to break the carousel motion
        float friction_factor_ = 0.01;
        // this is just to keep the tracking of the first frame after an interaction occurs
        bool is_dragging_ = false;
        // if the user released the carousel
        bool can_animate_ = false;
        bool is_animating_ = false;
        bool enabled_ = true;

        // debug variables
        MaterialPtr debug_material_;
        Material* debug_raw_material_ = nullptr;
        float3 half_extent_ = 0.1;
        Box debug_box_;
        const float4 FIRST_ELEMENT_COLOR_ = float4(1.0, 0.0, 0.0, 1.0);
        const float4 OTHER_ELEMENTS_COLOR_ = float4(1.0, 0.0, 1.0, 1.0);
        const float4 LAST_ELEMENT_COLOR_ = float4(0.0, 0.0, 1.0, 1.0);

        void FinishCarouselTranslation();
        NodeHandle GetNearObjectFromCenter();
        int GetNearObjectIndexFromCenter();
        void SubscribeCallbacks();
        void InitializeCarouselHierarchy();
        void RepositionItems(float lerp_time);
        void DebugSetup();

    public:
        void Setup();
        void Setup(const vector<NodeHandle>& carousel_elements);
        void Update(const FrameTime& frame_time);

        // add an element to the list of elements and reorganize the carousel
        void AddCarouselElement(const NodeHandle& element);
        // return the centralized node index
        int GetSelectedElementIndex();
        // return the centralized node
        NodeHandle GetSelectedElement();
    };
} // namespace xr::component
