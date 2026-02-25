#include "core/common/log.h"
#include "imp.h"
#include "native/components/starry_sky_noise/starry_sky_noise.h"
#include "native/components/starry_sky_noise/starry_sky_noise_assets.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    void Starry_Sky_Noise::Setup()
    {
        InitializeStarrySkyParameters();
    }

    void Starry_Sky_Noise::InitializeStarrySkyParameters()
    {
        data.star_tex_width   = state_.star_tex_width  .value_or(data.star_tex_width   );
        data.star_tex_height  = state_.star_tex_height .value_or(data.star_tex_height  );
        data.star_tex_scale   = state_.star_tex_scale  .value_or(data.star_tex_scale   );
        data.noise_tex_width  = state_.noise_tex_width .value_or(data.noise_tex_width  );
        data.noise_tex_height = state_.noise_tex_height.value_or(data.noise_tex_height );
        data.noise_tex_scale  = state_.noise_tex_scale .value_or(data.noise_tex_scale  );
        data.noise_speed      = state_.noise_speed     .value_or(data.noise_speed      );
        data.star_tex_alpha   = state_.star_tex_alpha  .value_or(data.star_tex_alpha   );
        data.noise_tex_alpha  = state_.noise_tex_alpha .value_or(data.noise_tex_alpha  );
        data.panel_dome       = state_.panel_dome      .value_or(data.panel_dome       );
        InitializeStarrySky();
    }

    void Starry_Sky_Noise::ResetStarrySkyParameters()
    {
        data.star_tex_width   = state_.star_tex_width  .emplace(initial_data.star_tex_width     );
        data.star_tex_height  = state_.star_tex_height .emplace(initial_data.star_tex_height    );
        data.star_tex_scale   = state_.star_tex_scale  .emplace(initial_data.star_tex_scale     );
        data.noise_tex_width  = state_.noise_tex_width .emplace(initial_data.noise_tex_width    );
        data.noise_tex_height = state_.noise_tex_height.emplace(initial_data.noise_tex_height   );
        data.noise_tex_scale  = state_.noise_tex_scale .emplace(initial_data.noise_tex_scale    );
        data.noise_speed      = state_.noise_speed     .emplace(initial_data.noise_speed        );
        data.star_tex_alpha   = state_.star_tex_alpha  .emplace(initial_data.star_tex_alpha     );
        data.noise_tex_alpha  = state_.noise_tex_alpha .emplace(initial_data.noise_tex_alpha    );
        data.panel_dome       = state_.panel_dome      .emplace(data.panel_dome                 );
        InitializeStarrySky();
    }

    void Starry_Sky_Noise::InitializeStarrySky()
    {
        auto material_future     =  GetView().GetAssetManager().LoadMaterial(assets::kStarrySkyNoiseMaterialCmat);
        auto null_material_future     =  GetView().GetAssetManager().LoadMaterial(assets::kNullMaterialCmat);
        auto model_future         =  GetView().GetAssetManager().LoadModel(assets::kMDLNight250219V3Glb);
        auto star_texture_future  =  GetView().GetAssetManager().LoadImage(assets::kTXStarTexture2048x1024Png);
        auto noise_texture_future =  GetView().GetAssetManager().LoadImage(assets::kTXMask512x512Png);

        material_future.Merge(null_material_future, model_future, star_texture_future, noise_texture_future).Then([this]
                (std::tuple<AssetPtr<imp::MaterialAsset>, AssetPtr<imp::MaterialAsset>, NodeHandle, AssetPtr<imp::ImageAsset>, AssetPtr<imp::ImageAsset>> result) {
                auto [material, null_material,  model_node, texture, texture_noise] = result;

            if (data.panel_dome == 1)
                current_mesh_index_ = 18;
            else
                current_mesh_index_ = 17;

            CreateStarrySky(material, null_material, model_node, texture, texture_noise, data.star_tex_width, data.star_tex_height, data.star_tex_scale,
                data.noise_tex_width, data.noise_tex_height, data.noise_tex_scale, data.noise_speed,data.star_tex_alpha, data.noise_tex_alpha, current_mesh_index_);

        }).KeptBy(this);
    }

    void Starry_Sky_Noise::CreateStarrySky(AssetPtr<MaterialAsset> material, AssetPtr<MaterialAsset> null_material, NodeHandle model_node,
            AssetPtr<ImageAsset> star_tex,AssetPtr<ImageAsset> mask_tex, float star_width, float star_height, float star_scale,
            float noise_width, float noise_height, float noise_scale,float noise_speed, float star_alpha, float noise_alpha, int model_index)
    {
        starry_sky_noise_node_ = model_node;
        starry_sky_noise_node_->SetName("I-Night");
        starry_sky_noise_node_->SetLocalPosition(float3(0.0,-0.5,0.0));
        auto render_model_scene = starry_sky_noise_node_->GetComponent<GltfRenderer>();

        imp::TextureFactory::Options options;
        options.min_filter = filament::backend::SamplerMinFilter::LINEAR_MIPMAP_LINEAR;
        options.mag_filter = filament::backend::SamplerMagFilter::LINEAR;
        options.wrap_mode  = filament::backend::SamplerWrapMode::REPEAT;
        auto tx       = GetView().GetTextureFactory().CreateTexture(*star_tex, options);
        auto tx_noise = GetView().GetTextureFactory().CreateTexture(*mask_tex, options);
        auto mat      = GetView().GetMaterialFactory().CreateMaterial(material);
        auto null_mat      = GetView().GetMaterialFactory().CreateMaterial(material);

        mat-> SetParameter("StarTex",std::move(tx));
        mat-> SetParameter("StarTexWidth",   star_width);
        mat-> SetParameter("StarTexHeight",  star_height);
        mat-> SetParameter("StarTexScale",   star_scale);

        mat-> SetParameter("NoiseTex",std::move(tx_noise));
        mat-> SetParameter("NoiseTexWidth",  noise_width);
        mat-> SetParameter("NoiseTexHeight", noise_height);
        mat-> SetParameter("NoiseTexScale",  noise_scale);

        mat-> SetParameter("NoiseSpeed",     noise_speed);

        mat-> SetParameter("StarTexAlpha",   star_alpha);
        mat-> SetParameter("NoiseTexAlpha",  noise_alpha);

        if (model_index == 17)
            render_model_scene->SetMaterialOverrideByIndex(std::move(null_mat), 18);

        render_model_scene->SetMaterialOverrideByIndex(std::move(mat), model_index);
    }

#if IMP_RUNTIME(DEV)
    // Customize Editor UI for this component
    void Starry_Sky_Noise::DrawEditorUi() 
    {
       if (ImGui::Button("Update effect"))
       {
           GetView().DestroyNode(starry_sky_noise_node_);
           InitializeStarrySkyParameters();
       }

        if (ImGui::Button("Reset effect"))
       {
           GetView().DestroyNode(starry_sky_noise_node_);
           ResetStarrySkyParameters();
       }
    }
#endif    
}
