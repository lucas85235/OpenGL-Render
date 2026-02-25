#include "native/components/UI/carousel/carousel.h"
#include "native/home_components_utils/common/math/lerp.h"

using namespace std;

namespace ix::samsung::homecomponents
{
    void Carousel::FinishCarouselTranslation()
    {
        is_dragging_ = false;
        selected_object_index_ = ((static_cast<int>(ceil(offset_index_)) % objects_amount_) + objects_amount_) % objects_amount_;
        offset_index_ = selected_object_index_;
        output::Warning("selected element: %d", selected_object_index_);
        output::Warning("offset index: %f", offset_index_);
        RepositionItems(0.0);

        // trigger the callback when an element was selected to the parent node
        on_element_selected_.element = carousel_elements_[selected_object_index_];
        on_element_selected_.element_index = selected_object_index_;
        on_element_selected_.last_selected_element = last_selected_carousel_element_;

        // update the last carousel element to deactivate the collider later
        last_selected_carousel_element_ = carousel_elements_[selected_object_index_];
        GetNode()->Send(on_element_selected_);
    }

    NodeHandle Carousel::GetNearObjectFromCenter()
    {
        float min_distance = 1000.0f;
        auto object = carousel_elements_[0];

        for (int i = 0; i < carousel_elements_.size(); ++i)
        {
            auto node_position = carousel_elements_[i]->GetWorldPosition();
            auto distance = MathUtils::GetDistance(node_position, GetNode()->GetWorldPosition());
            if (distance < min_distance)
            {
                min_distance = distance;
                object = carousel_elements_[i];
            }
        }

        return object;
    }

    int Carousel::GetNearObjectIndexFromCenter()
    {
        float min_distance = 1000.0f;
        int object_index = 0;

        for (int i = 0; i < carousel_elements_.size(); ++i)
        {
            auto node_position = carousel_elements_[i]->GetWorldPosition();
            auto distance = MathUtils::GetDistance(node_position, GetNode()->GetWorldPosition());
            if (distance < min_distance)
            {
                min_distance = distance;
                object_index = i;
            }
        }
        return object_index;
    }

    void Carousel::SubscribeCallbacks()
    {
        // this is only a workaround to avoid, on desktop builds, to interact with the carousel
        // while the user is rotating the camera
        GetView().GetDispatcher().Connect(
            [this](const KeyboardEvent& event) mutable
            {
                switch (event.type)
                {
                case KeyboardEventType::kOnDown:
                    {
                        if(event.key.code == VirtualKeyCode::VK_v && enabled_)
                        {
                            enabled_ = false;
                        }
                        break;
                    }

                case KeyboardEventType::kOnUp:
                    {
                        if(event.key.code == VirtualKeyCode::VK_v && !enabled_)
                        {
                            enabled_ = true;
                        }
                        break;
                    }
                }
            },
            this);

        interactable_component_->Connect([=](const InteractionPositionChanged& event)
        {
            switch (state_)
            {
            case SELECTED:
                {
                    if(!enabled_)
                    {
                        RepositionItems(snap_interpolation_seconds_);
                        return;
                    }
                    //* this first check serve as an initial delta calculation to avoid false values
                    if (is_dragging_)
                    {
                        is_dragging_ = false;
                        on_animation_started_.last_selected_element = last_selected_carousel_element_;
                        GetNode()->Send(on_animation_started_);
                        last_pointer_position_ = (event.position) * scroll_orientation_;
                        is_animating_ = true;
                    }

                    auto current_pointer_position = (event.position) * scroll_orientation_;
                    pointer_delta_position_ = (current_pointer_position - last_pointer_position_).x;
                    pointer_delta_position_ = imp::clamp(pointer_delta_position_, -max_movement_delta_, max_movement_delta_);

                    //* every time that the user moves his hands while dragging the offset float variable will change
                    offset_index_ += pointer_delta_position_ * drag_sensitivity_;
                    last_pointer_position_ = current_pointer_position;
                    selected_object_index_ = ((static_cast<int>(ceil(offset_index_)) % objects_amount_) + objects_amount_) % objects_amount_;

                    RepositionItems(snap_interpolation_seconds_);
                    break;
                }
            }
        });
        interactable_component_->Connect([=](const HitPositionChanged& event)
        {
            switch (state_)
            {
            case HOVER_MOVED:
                {
                    if (event.position != float3(0.0f))
                    {
                        last_hover_position_ = event.position;
                    }
                }
            }
        });

        interactable_component_->Connect([=](const InteractionStateChanged& event)
        {
            state_ = event.state;

            switch (state_)
            {
            case SELECTED:
                {
                    if(!enabled_) return;
                    if(IMP_RUNTIME(DEV) and DEBUG_CAROUSEL) output::Error("Selected");
                    is_dragging_ = true;
                    break;
                }


            case NORMAL:
                {
                    if(IMP_RUNTIME(DEV) and DEBUG_CAROUSEL) output::Error("NORMAL");
                    break;
                }

            case HOVER_ENTER:
                {
                    if(IMP_RUNTIME(DEV) and DEBUG_CAROUSEL) output::Error("HOVER_ENTER");
                    break;
                }

            case HOVER_MOVED:
                {
                    if(IMP_RUNTIME(DEV) and DEBUG_CAROUSEL) output::Error("HOVER_MOVED");
                    break;
                }

            case HOVER_EXIT:
                {
                    if(IMP_RUNTIME(DEV) and DEBUG_CAROUSEL) output::Error("HOVER_EXIT");
                    break;
                }

            case CLICKED_DOWN:
                {
                    if(IMP_RUNTIME(DEV) and DEBUG_CAROUSEL) output::Warning("CLICKED_DOWN");
                    break;
                }

            case CLICKED_UP:
                {
                    output::Warning("CLICKED_UP");

                    can_animate_ = true;
                    if(!enabled_) return;
                    //* If the carousel is not animating and the user clicked on left or right side of the carousel we need
                    //* to change for the next selected element based on the side that the click have occurred

                    /*
                     * TODO(Rafael): refactor this logic to handle carousel "Go to next" and "Go to previous" behavior.
                     * Currently, carousel is using an animation system that is not modular enough to support this kind of
                     * behavior. We need to think and implement some way to make this happen in a modular way support other
                     * kind of animations in the future.
                     */
                    if (!is_animating_)
                    {
                        last_click_position_ = last_hover_position_;
                        auto local_click_position = last_click_position_ - GetNode()->GetWorldPosition();
                        if (local_click_position.x > interaction_viewport_size_.x / 2.0)
                        {
                            //offset_index_--;
                            //RepositionItems(0.0);
                        }
                        else if (local_click_position.x < -interaction_viewport_size_.x / 2.0)
                        {
                            //offset_index_++;
                            //RepositionItems(0.0);
                        }
                        else
                        {
                            OnElementClicked on_element_clicked_;
                            on_element_clicked_.element_index = selected_object_index_;
                            GetNode()->Send(on_element_clicked_);
                        }
                    }
                    break;
                }
            }
        });
    }

    void Carousel::InitializeCarouselHierarchy()
    {
        carousel_node_ = GetView().CreateNode();
        carousel_node_->SetParent(GetNode());
        carousel_node_->SetName("Carousel");
        interactable_component_ = carousel_node_->AddComponent<Interactable>(1, 1, 1, 1);
        interactor_node_ = GetView().CreateNode();
        InteractorFactory::CreateInteractor(interactor_node_);
        carousel_collider_ = {.center = float3(0), .halfExtent = interaction_viewport_size_};
        carousel_node_->AddComponent<BoxCollider>(carousel_collider_);
        carousel_internal_holder_node_ = GetView().CreateNode();
        carousel_internal_holder_node_->SetName("HolderNode");
        carousel_internal_holder_node_->SetParent(carousel_node_);
    }

    void Carousel::AddCarouselElement(const NodeHandle& element)
    {
        element->SetParent(carousel_internal_holder_node_);
        carousel_elements_.push_back(element);
        selected_object_index_ = carousel_elements_.size() / 2;
        objects_amount_ = carousel_elements_.size();
        RepositionItems(0.0f);
    }

    int Carousel::GetSelectedElementIndex()
    {
        return selected_object_index_;
    }

    NodeHandle Carousel::GetSelectedElement()
    {
        return carousel_elements_[selected_object_index_];
    }

    void Carousel::RepositionItems(float lerp_time)
    {
        int items_count = carousel_elements_.size();
        float half_items_count = float(items_count) / 2.0;

        for (int i = 0; i < carousel_elements_.size(); i++)
        {
            auto local_position = carousel_elements_[i]->GetLocalPosition();
            float positionIndexDiff = i - offset_index_;

            // This is a kind of MOD function that prevents that negative values breaks the logic
            // don't change this if you don't know how it works :/
            int wrapped = floor(positionIndexDiff) + half_items_count;
            wrapped = ((wrapped % items_count) + items_count) % items_count;
            wrapped -= half_items_count;

            float angle = (wrapped * (carousel_cell_size_) / items_count) * 2.0 * M_PI;
            float3 new_pos = float3((elements_spacing_ + carousel_cell_size_ / 2.0 * items_count) * std::cos(angle + M_PI / 2.0), 0.0, 0.3 * std::sin(angle + M_PI / 2.0)) * radius_ - float3(0.0, 0.0, radius_ * 0.3);
            carousel_elements_[i]->SetLocalPosition(LerpUtils::ExponentialLerp(local_position, new_pos, 0.01, lerp_time));
        }
    }

    void Carousel::DebugSetup()
    {
        auto material_future = GetView().GetAssetManager().LoadMaterial(assets::kDebugMaterialCmat);
        debug_box_ = GetView().GetMeshFactory().CreateBox({.size = carousel_cell_size_, .center = {0, 0, 0}})->GetAabb();

        material_future.Then([&,this](AssetPtr<MaterialAsset> material)
        {
            debug_material_ = GetView().GetMaterialFactory().CreateMaterial(material); //debug variable
            AddCarouselElement(AddBoxModel("0", FIRST_ELEMENT_COLOR_));
            debug_material_ = GetView().GetMaterialFactory().CreateMaterial(material);
            AddCarouselElement(AddBoxModel("1", OTHER_ELEMENTS_COLOR_));
            debug_material_ = GetView().GetMaterialFactory().CreateMaterial(material);
            AddCarouselElement(AddBoxModel("2", OTHER_ELEMENTS_COLOR_));
            debug_material_ = GetView().GetMaterialFactory().CreateMaterial(material);
            AddCarouselElement(AddBoxModel("3", OTHER_ELEMENTS_COLOR_));
            debug_material_ = GetView().GetMaterialFactory().CreateMaterial(material);
            AddCarouselElement(AddBoxModel("4", OTHER_ELEMENTS_COLOR_));
            debug_material_ = GetView().GetMaterialFactory().CreateMaterial(material);
            AddCarouselElement(AddBoxModel("5", LAST_ELEMENT_COLOR_));

            SubscribeCallbacks();
        }).KeptBy(this);
    }

    void Carousel::Setup()
    {
        InitializeCarouselHierarchy();
        //DebugSetup();
        SubscribeCallbacks();
    }

    void Carousel::Setup(const vector<NodeHandle>& carousel_elements)
    {
        InitializeCarouselHierarchy();
        auto node_pos = GetNode()->GetWorldPosition();

        for (auto element : carousel_elements)
        {
            AddCarouselElement(element);
            selected_object_index_ = carousel_elements_.size() / 2;
        }

        SubscribeCallbacks();

    }

    void Carousel::Update(const FrameTime& frame_time)
    {
        /*
         * TODO(Rafael): Remake this animation logic to support animations that we have more control. This will be useful for
         * events like "Go to next element" and "Go to previous element" behaviors. The new animation method should be like
         * unity animation coroutines, because we can make a more modular and controllable behavior.
         */
        if (can_animate_)
        {
            is_animating_ = true;
            GetNode()->Send(on_animation_started_);
            offset_index_ += pointer_delta_position_ * drag_sensitivity_;
            //! always repositioning the items based on the current carousel index
            RepositionItems(snap_interpolation_seconds_);

            //! reduce the amount of movement delta for the next frame
            if (pointer_delta_position_ < 0.0f) pointer_delta_position_ += friction_factor_ * frame_time.GetDeltaSeconds();
            else if (pointer_delta_position_ > 0.0f) pointer_delta_position_ -= friction_factor_ * frame_time.GetDeltaSeconds();
            pointer_delta_position_ = std::clamp(pointer_delta_position_, -max_movement_delta_, max_movement_delta_);
            if(IMP_RUNTIME(DEV) and DEBUG_CAROUSEL) output::Error("delta_position: %f", pointer_delta_position_);

            if (abs(pointer_delta_position_) < 0.0001f)
            {
                pointer_delta_position_ = 0.0f;
                can_animate_ = false;
                is_animating_ = false;
                FinishCarouselTranslation();
            }
        }
    }

    NodeHandle Carousel::AddBoxModel(const string& name, float4 color)
    {
        NodeHandle box_node = GetView().CreateNode();
        box_node->SetName(name);
        NodeHandle shape = CreateShapeNode(GetView().GetMeshFactory().CreateBox({.size = half_extent_ * 2, .center = {0, 0, 0}}), color);
        shape->SetParent(box_node);
        return box_node;
    }

    void Carousel::PrintDebugMessage()
    {
        IMP_LOG(ERROR) << "Debug text! #######################################################################################################";
    }

    // Helper function to create a node with a shape.
    NodeHandle Carousel::CreateShapeNode(MeshPtr shape_mesh, const float4 color)
    {
        NodeHandle shape = GetView().CreateNode();
        ComponentHandle<RenderComponent> shape_renderer = shape->AddComponent<RenderComponent>(
            RenderComponent::FrustrumCullingMode::kDisabled);

        shape_renderer->SetMesh(std::move(shape_mesh));
        shape_renderer->SetMaterial(std::move(debug_material_));
        auto material = shape_renderer->GetMaterial();
        material->SetParameter("BaseColor", color);
        return shape;
    }
} // namespace ix::samsung::homecomponents
