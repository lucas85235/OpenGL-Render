#include "native/components/surface_base/surface_base.h"

namespace ix::samsung::homecomponents
{
    void SurfaceBase::Setup()
    {
        owner_node_ = GetNode();
        surface_mesh_ = owner_node_->CreateChildNode();
        surface_mesh_->SetName("New node 2");
        LoadSurfaceAssets();
    }

    void SurfaceBase::Update(const FrameTime &frame_time)
    {
        FindCurrentHeightAndFloor();
        FindFloor();
    }

    void SurfaceBase::FindFloor() {
        //gets the current node bounds
        auto bounding_box = owner_node_->GetComponent<BoxCollider>()->GetWorldBox();

        //calculate the down point to compare with object bounding box
        auto down_position = float3(bounding_box.center.x, bounding_box.getMin().y, bounding_box.center.z);
        auto near_down_point = down_position;

        //creates a ray from the near_down_point to down direction
        Ray world_ray = Ray(near_down_point, kDown);
        auto collision_manager = CollisionManager(&GetView());

        //set some masks to filter the hits from collision manager
        auto hits = collision_manager.IntersectAll(world_ray, Flags<CollisionMask>(EFFECT_MASK));
        auto ignored_hits = collision_manager.IntersectAll(world_ray, Flags<CollisionMask>(IGNORED_MASK));
        int current_state = -1;

        //if the incompatible node from hits was founded, hide the surface mesh
        if(!ignored_hits.empty() && ignored_hits[0].node.IsValid()){
            current_state = 0;
            surface_mesh_->SetEnabled(false);
        }

        //if the compatible node from hits was founded, shows the surface mesh
        else if (!hits.empty() && hits[0].node.IsValid() && hits[0].node != owner_node_)
        {
            current_state = 1;
            auto ray_hit = hits[0];
            surface_mesh_->SetEnabled(true);
            cur_height_ = ray_hit.distance;
            auto hit_position = ray_hit.world_point - model_offset_.y * surface_mesh_->GetWorldScale().y;
            surface_mesh_->SetWorldPosition(hit_position);
        }

        if (current_state_ != current_state)
        {
            current_state_ = current_state;
            OnEnableEvent.Trigger(surface_mesh_->IsEnabled(), piece_animation);
        }
    }

    void SurfaceBase::LoadSurfaceAssets() {
        auto material_future = GetView().GetAssetManager().LoadMaterial(data::kSurfaceMaterialCmat);
        auto hexagon_model_future = GetView().GetAssetManager().LoadModel(assets::kHexagonGlb);

        material_future.Merge(hexagon_model_future)
                .Then([=](std::tuple<AssetPtr<MaterialAsset>,NodeHandle> result)
                      {
                          auto [material, hexagon_node] = result;
                          auto model_renderer = hexagon_node->GetComponent<GltfRenderer>();
                          //calculates the vertical offset based on surface mesh bounds
                          model_offset_ = model_renderer->GetLocalBounds().halfExtent;
                          auto material_ptr = GetView().GetMaterialFactory().CreateMaterial(material);
                          material_ptr->SetParameter("SurfaceColor", surface_color_);
                          material_ptr->SetParameter("Alpha", surface_alpha_);
                          model_renderer->SetMaterialOverrideByIndex(std::move(material_ptr),0);
                          hexagon_node->GetChildren()[0]->GetChildren()[0]->GetComponent<GltfCollider>()->SetEnabled(false);
                          hexagon_node->SetName("New node");
                          surface_mesh_ = hexagon_node;
                      }).KeptBy(this);
    }

    void SurfaceBase::FindCurrentHeightAndFloor() {
        auto new_max_size = owner_node_->GetWorldScale().x;
        float new_height = lerp(new_max_size, 0.0f, cur_height_/max_height_);
        cur_base_scale_ = imp::clamp(new_height,0.0f,new_max_size);
        surface_mesh_->SetLocalScale(cur_base_scale_);
    }

    void SurfaceBase::Cleanup() {
        Component::Cleanup();
        OnEnableEvent.RemoveAllListeners();
        if(surface_mesh_.IsValid())
            GetView().DestroyNode(surface_mesh_);
    }
}  // namespace xr::component