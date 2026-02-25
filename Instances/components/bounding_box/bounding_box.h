#ifndef COMPONENTS_BOUNDING_BOX_H
#define COMPONENTS_BOUNDING_BOX_H

#include "imp.h"
#include "native/components/interaction/interaction.h"
#include "native/components/timer/timer.h"
#include "core/line_renderer/line_renderer.h"
#include "core/line_renderer/line_renderer.proto.imp.h"
#include "proto/components/bounding_box_state.proto.imp.h"
#include "native/components/interaction/interactable.h"
#include <string>

namespace ix::samsung::homecomponents {
    using namespace imp;
    using namespace std;

    struct BoundingBoxInteractionPhaseChanged : Event {
        State current_interaction_state;
    };

    class BoundingBox : public imp::Component {
    public:
        void Setup();

        void Setup(imp::NodeHandle model);

        void Setup(const float placeholder);

        void Update(const imp::FrameTime &frame_time);

        void OnActiveStatusChanged(bool is_active);

        imp::NodeHandle GetInteractionNode();

        void AdjustBoxesScale();

        void SetOffset(float3 offset);

        BoundingBoxInteractionPhaseChanged on_interaction_phase_changed_;

        Box GetBoundingBox();

    private:
        NodeHandle this_model_;
        NodeHandle interaction_node_;
        ComponentHandle<imp::CameraComponent> world_camera_;
        ComponentHandle<Timer> timer_;
        Box bounding_box_;
        bool first_activation_ = false;
        float interaction_distance_ = 2;
        float vertex_distance_from_opposing_vertex;
        float initial_z_coord_;
        float max_scale_percent_ = 1.5f;
        float min_scale_percent_ = 0.8f;
        float scale_threshold_ = 1;
        float previous_interaction_radius_delta;
        NodeHandle last_selected_node_;
        BoundingBoxState bounding_box_state_;
        vector<imp::NodeHandle> vertices_;
        vector<imp::NodeHandle> boxes_;
        vector<imp::ComponentHandle<LineRenderer>> lines_;
        vector<bool> hovered_status_;
        vector<ComponentHandle<Interactable>> interaction_components_;
        const std::string vertex_prefix_ = "Corner";
        const std::string edge_prefix_ = "Edge";

        float3 offset_ = kZero3;

        // This will result in rotating 90 degrees in 0.5 seconds
        const float rotation_duration_ = 0.5f;
        const int rotation_degrees_ = 90;

        // Opposing vertices mapping for scaling
        const int opposing_corners_[8] = {7, 6, 5, 4, 3, 2, 1, 0};

        // Vertex pairs for placing boxes on the edges for rotation
        const int vertex_pairs_[12][2] = {
                {0, 1},
                {1, 3},
                {3, 2},
                {2, 0},
                {4, 5},
                {5, 7},
                {7, 6},
                {6, 4},
                {0, 4},
                {1, 5},
                {2, 6},
                {3, 7}
        };

        // Local rotation for vertex models
        const imp::float3 vertex_rotations[8] = {
                imp::float3(270, 0, 0),
                imp::float3(270, 90, 0),
                imp::float3(0, 0, 0),
                imp::float3(0, 90, 0),
                imp::float3(180, 90, 90),
                imp::float3(270, 180, 0),
                imp::float3(0, 270, 0),
                imp::float3(0, 180, 0),
        };

        // Normal and highlight colors for handles; colors are in linear space
        // Color values are normalized, so we just divide the RGB value by 255
        const float3 normal_handle_color_ = imp::float3(250.0f / 255.0f, 250.0f / 255.0f,
                                                        250.0f / 255.0f); // #FAFAFA - 250, 250, 250
        const float3 highlight_handle_color_ = imp::float3(29.0f / 255.0f, 133.0f / 255.0f,
                                                           200.0f / 255.0f); // #1D85C8 - 29, 133, 200

        NodeHandle AddInteractionHandle(std::string prefix, imp::float3 location, imp::float3 box_scale,
                                        ::imp::resources::ResourceDefinition model_url);

        NodeHandle CreateShapeNode(imp::MeshPtr shape_mesh);

        Box Union(std::optional<imp::Box> a, const imp::Box &b);

        void ChangeRendersVisibility(bool visibility);

        void CreateLine(float3 start, float3 end);

        void SetModel(imp::NodeHandle model);

        void CreateBoundingBox();

        float GetDistance(float3 vec1, float3 vec2);

        float Length(float3 vec);

        void BindDragGesture();

        bitset<21> interaction_mask_;

        void EditLineColor(float4 color);

        float3 CrossProduct(float3 v1, float3 v2);

        int FindNode(std::vector<ComponentHandle<Interactable>> components, NodeHandle search_node);

        float GetDistance(std::optional<float2> vec1, std::optional<float2> vec2);

        optional<imp::Box> GetBounds(imp::NodeHandle node, std::optional<imp::Box> result = std::nullopt);

        void SetMaterialBaseColor(imp::NodeHandle node, imp::float3 color);

        void SetMaterialAlpha(imp::NodeHandle node, float alpha);

        Material *GetMaterialFromGltf(NodeHandle node);


    public:
        using IsfInfo = imp::IsfInfo<&BoundingBox::bounding_box_state_>;
    };
}
#endif  // COMPONENTS_BOUNDING_BOX_H
