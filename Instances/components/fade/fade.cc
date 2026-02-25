#include "fade.h"

using namespace imp;

namespace ix::samsung::homecomponents {

    void Fade::Setup(float time_fade) {
        duration_ = time_fade;
        camera_node_ = GetView().GetCameraManager().GetCamera()->GetNode();

        GetView().GetAssetManager().LoadMaterial(data::kColorMaterialCmat).Then([this](AssetPtr<imp::MaterialAsset> material){
            mask_node_ = AddMaskLocation("mask", float3(0,0,-0.05f), std::move(material));
            mask_node_->SetParent(camera_node_);
        }).KeptBy(this);
    }

    void Fade::Update(const FrameTime &frame_time) {
        if (is_active_fade){
            elapsed_ += frame_time.GetDeltaSeconds();
            current = ((-direction_ * (cos((elapsed_ / duration_) * M_PI * 0.5)) + direction_)) + initial_;
            material_sphere_->SetParameter("Alpha", current);

            if (initial_ - end_ > 0) {
                if (current < (end_ - tolerance_)) {
                    current = end_;
                    onFadeFinished();
                }
            }
            else {
                if (current > (end_ - tolerance_)) {
                    current = end_;
                    onFadeFinished();
                }
            }
        }
    }

    void Fade::onFadeFinished() {
        on_fade_finished_.is_fade_in = is_fade_in_;
        FadeReset();
        GetNode()->Send(on_fade_finished_);
    }

    void Fade::FadeReset() {
        is_active_fade = false;
        FadeController(false, 0, 0, 0, false);
    }

    void Fade::FadeController(bool is_fade_in, float initial, float end ,float duration, bool is_active) {
        is_fade_in_ = is_fade_in;
        initial_ = initial;
        end_ = end;
        elapsed_ = 0;
        current = initial;

        if (initial_ - end_ < 0)
            direction_ = 1;
        else
            direction_ = -1;

        if (duration < tolerance_)
            duration_ = 0.05;
        else
            duration_ = duration;

        is_active_fade = is_active;
    }

    void Fade::FadeOut() {
        FadeController(false, 0, 1, 1, true);
    }

    void Fade::FadeIn() {
        FadeController(true, 1, 0, 1, true);
    }

    NodeHandle Fade::AddMaskLocation(std::string prefix, float3 location, AssetPtr<imp::MaterialAsset> material)
    {
        NodeHandle mask_node = GetView().CreateNode();
        mask_node->SetName(prefix);
        mask_node->SetLocalPosition(location);
        mask_node->SetLocalScale(0.25f);

        NodeHandle shape = CreateShapeNode(
                GetView().GetMeshFactory().CreateQuad(), material);
        shape->SetParent(mask_node);
        return mask_node;
    }

    NodeHandle Fade::CreateShapeNode(MeshPtr shape_mesh, AssetPtr<imp::MaterialAsset> material)
    {
        NodeHandle shape = GetView().CreateNode();
        ComponentHandle<RenderComponent> shape_renderer =
                shape->AddComponent<RenderComponent>(
                        RenderComponent::FrustrumCullingMode::kDisabled);

        auto shape_material = GetView().GetMaterialFactory().CreateMaterial(material);
        shape_renderer->SetMesh(std::move(shape_mesh));
        shape_renderer->SetMaterial(std::move(shape_material));
        shape_renderer->SetChannel(3);
        shape_renderer->SetPriority(-1);
        material_sphere_ = shape_renderer->GetMaterial(0);
        return shape;
    }

}  // namespace ix::samsung::homecomponents
