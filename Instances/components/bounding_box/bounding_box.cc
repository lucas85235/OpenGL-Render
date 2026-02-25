#include <string>
#include <thread>
#include "native/components/timed_spin/timed_spin.h"
#include "bounding_box.h"
#include "core/math/math.h"
#include "native/components/bounding_box/bounding_box_assets.h"

#if IMP_RUNTIME(DEV)
#include "dear_imgui/imgui.h"
#endif

using namespace imp;

namespace ix::samsung::homecomponents {
    void BoundingBox::Setup() {
    }

    void BoundingBox::Setup(NodeHandle model) {
        auto renderer = model->GetComponent<GltfRenderer>();
        if (!renderer) {
            output::Fatal("Bounding Box: model node should have a GltfRenderer component!");
            return;
        }
        SetModel(model);

        // This component must be present for the rotation interaction
        GetNode()->AddComponent<TimedSpin>();
        timer_ = GetNode()->AddComponent<Timer>();
    }

    void BoundingBox::Setup(const float placeholder) {
        bounding_box_state_.placeholder = placeholder;
    }

    void BoundingBox::Update(const FrameTime &frame_time) {
        for (int i = 0; i < 8; i++) {
            vertices_[i]->SetLocalRotation(QuatFromEuler(vertex_rotations[i]));
        }
    }

    imp::NodeHandle BoundingBox::GetInteractionNode() {
        return interaction_node_;
    }

    // When the component is activated for the first time,
    // create the bounding box
    void BoundingBox::OnActiveStatusChanged(bool is_active) {
        if (is_active && this_model_) {
            if (first_activation_) return;
            first_activation_ = true;
            world_camera_ = GetView().GetCameraManager().GetCamera();
            CreateBoundingBox();
            BindDragGesture();
        }
    }

    void BoundingBox::SetModel(NodeHandle model) {
        this_model_ = model;
        this_model_->SetParent(GetNode());
        this_model_->SetName("BoundingBoxModel");
    }

    Box BoundingBox::Union(std::optional<Box> a, const Box &b) {
        if (!a.has_value()) return b;
        return a->unionSelf(b);
    }

    void BoundingBox::CreateBoundingBox() {
        float current_scale = GetNode()->GetWorldScale().x;
        max_scale_percent_ = current_scale * max_scale_percent_;
        min_scale_percent_ = current_scale * min_scale_percent_;

        // First, get the bounds from the model at local position (0,0,0)
        this_model_->SetLocalPosition(float3(0));
        Box model_box = *GetBounds(this_model_);

        model_box.center = model_box.center * this_model_->GetWorldScale();
        model_box.halfExtent = model_box.halfExtent * this_model_->GetWorldScale();
        model_box.halfExtent += imp::float3(0.0001, 0.0001, 0.0001);

        // Then, copy the box and recenter it to be the bounding box on the parent node
        bounding_box_ = model_box;
        bounding_box_.center = float3(0);

        // Add a BoxCollider using the obtained bounding box
        GetNode()->AddComponent<BoxCollider>(bounding_box_)->SetMask(CollisionMask::kStatic);

        // Calculate the difference between boxes to be the model offset
        float3 model_offset = bounding_box_.center - model_box.center;

        // Adjust model local position to be in the bounding box center
        // The model offset and local position should be recalculated
        // when the bounding box is scaled
        this_model_->SetLocalPosition(model_offset);


        // Expanded Bounding box for placing handles, 0.0016 on each side
        Box expanded_box = bounding_box_;
        expanded_box.halfExtent += float3(0.0016);
        float3 min = expanded_box.getMin();
        float3 max = expanded_box.getMax();

        //add the drag movement logic with this interaction component
        interaction_node_ = GetNode()->CreateChildNode();
        interaction_node_->SetName("InteractionNode");
        interaction_node_->AddComponent<BoxCollider>()->SetBox(bounding_box_);
        interaction_components_.push_back(interaction_node_->AddComponent<Interactable>(1,1,1,1));

        scale_threshold_ = 1 / (bounding_box_.halfExtent.x * 3);

        float3 corners[8] = {
                {max.x, min.y, min.z}, //1
                {min.x, min.y, min.z}, //0
                {max.x, max.y, min.z}, //2
                {min.x, max.y, min.z}, //3
                {max.x, min.y, max.z}, //5
                {min.x, min.y, max.z}, //4
                {max.x, max.y, max.z}, //6
                {min.x, max.y, max.z}, //7

                /*
                    3________2
                  . |       .|
                7__________6 |
                |  |       | |
                |  0_______| 1
                | .        |.
                4__________5
                */
        };

        // Adding vertex boxes
        for (int i = 0; i < 8; i++) {
            // Adding node with vertex model at corner
            std::stringstream ss;
            ss << vertex_prefix_ << i;
            // Scale vertex boxes by 40% (1.4)
            auto vertex = AddInteractionHandle(ss.str(), corners[i], float3(1.4), data::kSMVertexUndGlb);
            boxes_.push_back(vertex);
            vertices_.push_back(vertex);
            interaction_components_.push_back(boxes_[i]->AddComponent<Interactable>(1,1,0,1));

            // Adjust node rotation to match the model
            vertex->SetLocalRotation(QuatFromEuler(vertex_rotations[i]));

            // Adding hover highlight on vertices
            vertex->Connect([this, vertex](const InteractionStateChanged &event) mutable {
                if (event.state == State::HOVER_ENTER || event.state == State::HOVER_MOVED) {
                    SetMaterialBaseColor(vertex, highlight_handle_color_);
                } else if (event.state == State::NORMAL || event.state == State::HOVER_EXIT) {
                    SetMaterialBaseColor(vertex, normal_handle_color_);
                }
            });
        }

        // Adding edge boxes
        for (int i = 0; i < 12; i++) {
            // Edge boxes are located on the middle point of each vertex pair
            float3 middle = (corners[vertex_pairs_[i][0]] + corners[vertex_pairs_[i][1]]) / 2;
            CreateLine(corners[vertex_pairs_[i][0]], corners[vertex_pairs_[i][1]]);
            std::stringstream ss;
            quatf edge_rotation;
            ss << edge_prefix_ << i;

            if (corners[vertex_pairs_[i][0]].x != corners[vertex_pairs_[i][1]].x) {
                edge_rotation = QuatFromEuler(float3(0, 0, 90));
            } else if (corners[vertex_pairs_[i][0]].z != corners[vertex_pairs_[i][1]].z) {
                edge_rotation = QuatFromEuler(float3(90, 0, 0));
            }

            std::string node_name = ss.str();

            // Scale by 5 on X and Z axes
            auto edge = AddInteractionHandle(ss.str(), middle, float3(5, 1, 5), data::kSMEdgesUndGlb);
            boxes_.push_back(edge);

            edge->SetLocalRotation(edge->GetLocalRotation() * edge_rotation);

            // Add interaction component for edge
            interaction_components_.push_back(edge->AddComponent<Interactable>(0,1,0,0));
            edge->Connect([this, edge](const InteractionStateChanged &event) mutable {
                if (event.state == State::CLICKED_DOWN) {
                    auto spin = GetNode()->GetComponent<TimedSpin>();
                    if (spin) {
                        auto camera_world_pos = world_camera_->GetNode()->GetWorldPosition();
                        auto camera_world_up = world_camera_->GetNode()->GetWorldRotation() * kUp;
                        auto parent_world_pos = edge->GetParent()->GetWorldPosition();
                        auto edge_world_pos = edge->GetWorldPosition();

                        // Determine which rotation axis
                        float3 axis_world = edge->GetWorldRotation() * kUp; //camera_world_up; TODO: Test of device
                        float3 axis_local = edge->GetLocalRotation() * kUp;

                        // Determine forward vector from parent node to camera
                        auto forward = camera_world_pos - parent_world_pos;

                        // Normal vector of plane
                        auto n = cross(forward, axis_world);

                        // General equation of a plane
                        float a = n.x;
                        float b = n.y;
                        float c = n.z;
                        float d = -camera_world_pos.x * n.x - camera_world_pos.y * n.y - camera_world_pos.z * n.z;

                        // Evaluate a point in tha plane equation
                        auto val = dot(float4(a, b, c, d),
                                       float4(edge_world_pos.x, edge_world_pos.y, edge_world_pos.z, 1));

                        // Sign of rotation
                        int rotation_sign = -sign(val);

                        spin->DoSpin(axis_local, rotation_duration_, rotation_sign * rotation_degrees_);
                    }
                }

                // Adding hover highlight on edges
                if (event.state == State::HOVER_ENTER || event.state == State::HOVER_MOVED) {
                    SetMaterialBaseColor(edge, highlight_handle_color_);
                } else if (event.state == State::NORMAL || event.state == State::HOVER_EXIT) {
                    SetMaterialBaseColor(edge, normal_handle_color_);
                }
            });
        }

        //a loop for all vertex boxes
        for (int i = 0; i < 8; i++) {
            boxes_[i]->Connect([this, i](const InteractionStateChanged &event) mutable {
                if (event.state != State::SELECTED) return;
                //get the initial position of camera, interacted box and it`s opposing box
                float3 camera_position = world_camera_->GetNode()->GetWorldPosition();
                float3 vertex_position = boxes_[i]->GetWorldPosition();
                float3 opposing_vertex_position = boxes_[opposing_corners_[i]]->GetWorldPosition();

                //get`s the interaction radius based on interacted box
                interaction_distance_ = GetDistance(camera_position, vertex_position);

                //translate the z coord of the opposing box to the interacted z position coord
                initial_z_coord_ = vertex_position.z;
                opposing_vertex_position.z = initial_z_coord_;
                vertex_distance_from_opposing_vertex = GetDistance(vertex_position, opposing_vertex_position);

            }, this);

            boxes_[i]->Connect([this, i](const InteractionPositionChanged &event) mutable {
                //verifies if this box is selected
                ComponentHandle<Interactable> interaction_component = boxes_[i]->GetComponent<Interactable>();
                if (interaction_component->GetInteractionState() != State::SELECTED) return;

                float3 new_position = event.position;
                float3 opposing_vertex_position = boxes_[opposing_corners_[i]]->GetWorldPosition();

#if IMP_PLATFORM(ANDROID)
                if(GetView().GetRegistry().Get<XrActionController>().ok()){
                    new_position = event.position;
                }
#endif

                EditLineColor(float4(highlight_handle_color_, 1));

                for (auto box: boxes_) {
                    SetMaterialBaseColor(box, highlight_handle_color_);
                }

                //translate the z coord of the opposing box to the interacted z position coord
                opposing_vertex_position.z = initial_z_coord_;
                new_position.z = initial_z_coord_;

                float new_distance_from_selected_box = GetDistance(new_position, opposing_vertex_position);
                float distance_delta =
                        (new_distance_from_selected_box - vertex_distance_from_opposing_vertex) * scale_threshold_;
                float3 new_scale = this_model_->GetParent()->GetWorldScale() + float3(1, 1, 1) * distance_delta;

                //the new scale must be inside the limits
                if (new_scale.x > max_scale_percent_ || new_scale.x < min_scale_percent_) return;

                this_model_->GetParent()->SetWorldScale(new_scale);

                //updates the current distance
                vertex_distance_from_opposing_vertex = new_distance_from_selected_box;

                //normalize all box scales based on initial scale
                for (auto box: boxes_) {
                    box->SetWorldScale(float3(1));
                }

                for (auto line: lines_) {
                    Material *material = line->GetNode()->GetComponent<RenderComponent>()->GetMaterial();
                    float width = 0.001 / GetNode()->GetWorldScale().x;
                    material->SetParameter("width", width);
                }
            }, this);
        }
    }

    // Add a BoxCollider with a procedural cube on the location and size requested
    void BoundingBox::BindDragGesture() {
        /*interaction_node_->Connect([this](const InteractionDistanceChanged &event) mutable
        {
            auto component = interaction_node_->GetComponent<Interaction>();
            if (component->GetInteractionState() != State::SELECTED) return;
            interaction_distance_ += event.interaction_radius_delta;
        }, this);*/

        //update drag
        interaction_node_->Connect([this](const InteractionPositionChanged &event) mutable {
            auto component = interaction_node_->GetComponent<Interactable>();
            if (component->GetInteractionState() != State::SELECTED) return;

            GetNode()->SetWorldPosition(event.position + offset_);

            /*for (auto line: lines_) {
                Material *material = line->GetNode()->GetComponent<RenderComponent>()->GetMaterial();
                float width = 0.001 / GetNode()->GetWorldScale().x;
                material->SetParameter("width", width);
            }*/

        }, this);

        GetNode()->Connect([this](const TimerCompleted &event) mutable {
            ChangeRendersVisibility(false);
        }, this);

        //update drag
        GetNode()->Connect([this](const InteractionStateChanged &event) mutable {
            switch (event.state) {
                case State::NORMAL:
                case State::HOVER_EXIT:
                    interaction_mask_[FindNode(interaction_components_, event.GetOriginatingNode())] = false;
                    if (interaction_mask_.none()) {
                        timer_->DoTimer(.5);
                        EditLineColor(float4(0));
                        for (auto box: boxes_) {
                            SetMaterialBaseColor(box, normal_handle_color_);
                        }
                        on_interaction_phase_changed_.current_interaction_state = State::NORMAL;
                        GetNode()->Send(on_interaction_phase_changed_);
                    }
                    break;
                case State::HOVER_ENTER:
                case State::HOVER_MOVED:
                    timer_->StopTimer();
                    interaction_mask_[FindNode(interaction_components_, event.GetOriginatingNode())] = true;
                    ChangeRendersVisibility(true);

                    break;
                case State::SELECTED:
                    timer_->StopTimer();
                    interaction_mask_[FindNode(interaction_components_, event.GetOriginatingNode())] = true;
                    last_selected_node_ = event.GetOriginatingNode();
                    EditLineColor(float4(highlight_handle_color_, 1));
                    interaction_distance_ = distance(last_selected_node_->GetWorldPosition(),
                                                     world_camera_->GetNode()->GetWorldPosition());

                    on_interaction_phase_changed_.current_interaction_state = State::SELECTED;
                    GetNode()->Send(on_interaction_phase_changed_);

                    for (auto box: boxes_) {
                        SetMaterialBaseColor(box, highlight_handle_color_);
                    }
                    break;
            }
        }, this);
    }

    // Add a BoxCollider on the location with the model provided, using the scaled box from the model
    NodeHandle BoundingBox::AddInteractionHandle(const std::string prefix, float3 location, float3 box_scale,
                                                 ::imp::resources::ResourceDefinition model_url) {
        NodeHandle box_node = GetNode()->CreateChildNode();

        box_node->SetName(prefix);
        box_node->SetParent(GetNode());
        box_node->SetLocalPosition(location);

        auto gltf_future = GetView().GetAssetManager().LoadGltfAsset(model_url);
        auto material_future = GetView().GetAssetManager().LoadMaterial(data::kBoundsShaderCmat);
        gltf_future.Merge(material_future)
                .Then([this, box_node, box_scale](std::tuple<AssetPtr<GltfAsset>, AssetPtr<MaterialAsset>> result) {
                    auto [gltf, material] = std::move(result);

                    // Adding GltfRenderer to node with the provided gLTF
                    ComponentHandle<GltfRenderer> renderer = box_node->AddComponent<GltfRenderer>(gltf);
                    renderer->SetEnabled(false);
                    box_node->GetChildren()[0]->GetChildren()[0]->SetLocalPosition(float3(0));

                    // Override GltfRenderer material with custom one, set initial parameters
                    MaterialPtr material_ptr = GetView().GetMaterialFactory().CreateMaterial(material);
                    material_ptr->SetParameter("BaseColor", filament::RgbType::LINEAR, normal_handle_color_);
                    material_ptr->SetParameter("Alpha", 1.0f);
                    renderer->SetMaterialOverrideByIndex(std::move(material_ptr), 0);

                    // Add box collider based on the original model collider
                    Box model_box = *GetBounds(box_node);
                    model_box.halfExtent.x *= box_scale.x;
                    model_box.halfExtent.y *= box_scale.y;
                    model_box.halfExtent.z *= box_scale.z;
                    box_node->AddComponent<BoxCollider>(model_box);
                }).KeptBy(this);

        return box_node;
    }

    // Obtain a Box as the union of all the bounds of the model meshes
    std::optional<Box> BoundingBox::GetBounds(NodeHandle node, std::optional<Box> result) {
        ComponentHandle<GltfMesh> gltf_mesh = node->GetComponent<GltfMesh>();
        if (gltf_mesh) {
            result = Union(result, gltf_mesh->GetLocalBounds());
        }

        for (NodeHandle child: node->GetChildren()) {
            result = GetBounds(child, result);
        }

        return result;
    }

    // Helper function to create a node with a shape.
    NodeHandle BoundingBox::CreateShapeNode(MeshPtr shape_mesh) {
        NodeHandle shape = GetNode()->CreateChildNode();
        ComponentHandle<RenderComponent> shape_renderer =
                shape->AddComponent<RenderComponent>(
                        RenderComponent::FrustrumCullingMode::kDisabled);
        shape_renderer->SetMesh(std::move(shape_mesh));
        return shape;
    }

    imp::Material *BoundingBox::GetMaterialFromGltf(NodeHandle node) {
        auto renderer = node->GetComponent<GltfRenderer>();
        if (renderer) {
            return renderer->GetMaterialOverrideByIndex(0);
        } else {
            return nullptr;
        }
    }

    void BoundingBox::SetMaterialBaseColor(NodeHandle node, float3 color) {
        auto material = GetMaterialFromGltf(node);
        if (material) {
            material->SetParameter("BaseColor", filament::RgbType::LINEAR, color);
        }
    }

    void BoundingBox::SetMaterialAlpha(NodeHandle node, float alpha) {
        auto material = GetMaterialFromGltf(node);
        if (material) {
            material->SetParameter("Alpha", alpha);
        }
    }

    float BoundingBox::GetDistance(float3 vec1, float3 vec2) {
        return sqrt(pow(vec1.x - vec2.x, 2) + pow(vec1.y - vec2.y, 2) + pow(vec1.z - vec2.z, 2));
    }

    float BoundingBox::GetDistance(std::optional<float2> vec1, std::optional<float2> vec2) {
        return sqrt(pow(vec1->x - vec2->x, 2) + pow(vec1->y - vec2->y, 2));
    }

    float BoundingBox::Length(float3 vec) {
        return sqrt(pow(vec.x, 2) + pow(vec.y, 2) + pow(vec.z, 2));
    }

    int BoundingBox::FindNode(std::vector<ComponentHandle<Interactable>> components, NodeHandle search_node) {
        for (int i = 0; i < components.size(); i++) {
            if (components[i].IsValid() && search_node->GetComponent<Interactable>().IsValid() &&
                components[i] == search_node->GetComponent<Interactable>()) {
                return i;
            }
        }
        return 0;
    }

    void BoundingBox::ChangeRendersVisibility(bool visibility) {
        for (auto box: boxes_) {
            ComponentHandle<GltfRenderer> renderer = box->GetComponent<GltfRenderer>();
            if (renderer) {
                renderer->SetEnabled(visibility);
            }
        }
    }

    void BoundingBox::CreateLine(float3 start, float3 end) {
        float4 color(1, 1, 1, 0);
        float line_tickness = 0.001;
        float line_feather = 100;
        NodeHandle owner = GetNode();
        TQuaternion<float> node_rotation = owner->GetWorldRotation();
        float pitch = node_rotation.x;
        float yaw = node_rotation.y;
        float3 right_vector(cos(yaw), 0, -sin(yaw));
        float3 up_vector(sin(pitch) * sin(yaw), cos(pitch), sin(pitch) * cos(yaw));
        NodeHandle line = GetNode()->CreateChildNode();
        line->SetParent(GetNode());
        line->SetName("line");

        //the edge mid point
        float3 edge_mid_point = (end - start) / 2;

        //normal vector based on an edge and a face
        float3 face_vector = float3(0);

        if (abs(edge_mid_point.z) > 0) {
            face_vector = float3(0, 1, 0);
        } else if (abs(edge_mid_point.x) > 0) {
            face_vector = float3(0, 1, 0);
        } else if (abs(edge_mid_point.y) > 0) {
            face_vector = float3(1, 0, 0);
        }

        //creates a line definition to configure the line renderer correctly
        LineRendererState line_definition;
        line_definition.points = {start, end};
        line_definition.normal = face_vector;
        line_definition.color = color;
        line_definition.width = line_tickness;
        line_definition.feather = line_feather;
        line_definition.wrap = false;
        //awaits the line render be ready
        line_definition.end_cap_shape = LineCapShape::LINE_CAP_SHAPE_ROUNDED;

        line->AddComponentWithState<LineRenderer>(line_definition).Then(
                [this](ComponentHandle<LineRenderer> line_component) {
                    lines_.push_back(line_component);
                }).KeptBy(this);

        NodeHandle line2 = GetNode()->CreateChildNode();
        line2->SetParent(GetNode());
        line2->SetName("line");

        if (abs(edge_mid_point.z) > 0) {
            face_vector = float3(1, 0, 0);
        } else if (abs(edge_mid_point.x) > 0) {
            face_vector = float3(0, 0, 1);
        } else if (abs(edge_mid_point.y) > 0) {
            face_vector = float3(0, 0, 1);
        }

        line_definition.normal = face_vector;

        line2->AddComponentWithState<LineRenderer>(line_definition).Then(
                [this](ComponentHandle<LineRenderer> line_component) {
                    lines_.push_back(line_component);
                }).KeptBy(this);

        line2->SetLocalPosition(float3(0));

    }

    void BoundingBox::EditLineColor(float4 color) {
        for (auto line: lines_) {
            line->SetColor(color);
        }
    }

    float3 BoundingBox::CrossProduct(float3 v1, float3 v2) {
        float3 result(
                v1.y * v2.z - v1.z * v2.y,
                -v1.x * v2.z - v1.z * v2.x,
                v1.x * v2.y - v1.y * v2.x
        );
        return result;
    }

    void BoundingBox::AdjustBoxesScale() {
        for (auto box: boxes_) {
            box->SetWorldScale(float3(1));
        }
    }

    void BoundingBox::SetOffset(float3 offset) {
        this->offset_ = offset;
    }

    Box BoundingBox::GetBoundingBox() {
        return bounding_box_;
    }

} // namespace ix::samsung::homecomponents