#include "imp.h"
#include "core/common/log.h"
#include "native/components/water_wave/assets.h"
#include "native/components/particle_instances/particle_instances.h"
#include "water_wave.h"

#include "core/lighting/image_based_lighting_asset_iblprefilter_loader.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    void WaterWave::Setup()
    {
        Initialize();
    }
    void WaterWave::Setup(WaterWaveParameters parameters)
    {
        state_.sizeOfGrid = parameters.sizeOfGrid;
        state_.velocity = parameters.velocity;
        state_.radius = parameters.radius;
        state_.frequency = parameters.frequency;
        state_.amplitude = parameters.amplitude;
        state_.textureName = parameters.textureName;
        Initialize();
    }

    void WaterWave::Initialize()
    {
        auto material_future = GetView().GetAssetManager().LoadMaterial(assets::kWaterWaveMaterialCmat);

        auto runtime_ibl_loader = imp::IblPrefilterLoader(int2{1024, 1024});
        auto reflection_future = GetView().GetAssetManager().LoadAsset<imp::ImageBasedLightingAsset>(state_.textureName, runtime_ibl_loader);

        material_future.Merge(reflection_future)
            .Then([this](std::tuple<AssetPtr<imp::MaterialAsset>, imp::AssetPtr<imp::ImageBasedLightingAsset>> result) {
                auto [material, reflection_ibl_asset] = result;

                reflection_ibl_asset_ = reflection_ibl_asset;

                imp::TextureFactory::Options options;
                options.min_filter = filament::backend::SamplerMinFilter::NEAREST_MIPMAP_LINEAR;
                options.mag_filter = filament::backend::SamplerMagFilter::LINEAR;
                options.wrap_mode = filament::backend::SamplerWrapMode::MIRRORED_REPEAT;

                CreateQuadSettings settings;
                settings.size = {50, 50};

                auto panelMesh = GetView().GetMeshFactory().CreatePanel(settings);
                auto node = GetNode();
                node->SetLocalPosition(imp::float3(0,-.6,-10.));
                node->SetLocalRotation(QuatFromEuler(float3(-90,0, 0)));

                ComponentHandle<RenderComponent> renderer_model = node->AddComponent<RenderComponent>(RenderComponent::FrustrumCullingMode::kDisabled);
                renderer_model->SetMesh(std::move(panelMesh));

                auto mat = GetView().GetMaterialFactory().CreateMaterial(material);
                mat->SetParameter("SkyBoxCube", reflection_ibl_asset_->GetSkyboxCubemap());
                
                material_ptr_ = mat.get();

                renderer_model->SetMaterial(std::move(mat));

                OnIsfStateChanged();

            }).KeptBy(this);
    }

    // this another approach using particles instances (It is not being used for this purpose.)
    void WaterWave::InitializeWithMultiplesShaders()
    {
        auto material_future = GetView().GetAssetManager().LoadMaterial(assets::kWaterWaveMaterialCmat);
        auto texture_future = GetView().GetAssetManager().LoadImage(assets::kTXWaterWaveJpeg); // for test with particle instance



        material_future.Merge(texture_future)
            .Then([this](std::tuple<AssetPtr<imp::MaterialAsset>, imp::AssetPtr<imp::ImageAsset>> result) {
                auto [material, texture_texture] = result;

                // particle instance

                auto particle_instance_node_ = GetView().CreateNode();
                particle_instance_node_->SetParentKeepWorldTransform(GetNode());
                auto params = ParticleInstancesParams();
                params.amount = 30;
                params.material = GetView().GetMaterialFactory().CreateMaterial(material);

                imp::TextureFactory::Options options;
                options.min_filter = filament::backend::SamplerMinFilter::NEAREST_MIPMAP_LINEAR;
                options.mag_filter = filament::backend::SamplerMagFilter::LINEAR;
                options.wrap_mode = filament::backend::SamplerWrapMode::MIRRORED_REPEAT;


                params.material->SetParameter("Texture", GetView().GetTextureFactory().CreateTexture(*texture_texture, options));
                MeshQuad cube;
                params.mesh = &cube;
                // params.velocity = twinkleStarsData_.velocity;
                // params.radius = twinkleStarsData_.radius;
                // params.size = twinkleStarsData_.size;
                // params.position = twinkleStarsData_.position;
                // params.color = twinkleStarsData_.color;

                params.positionFunction = [](float index_) {
                    float size = 10.;
                    std::uniform_real_distribution<float> rand_x(-size, size);
                    std::uniform_real_distribution<float> rand_y(0., size);
                    std::uniform_real_distribution<float> rand_z(-3.*size, -2.*size);

                    std::mt19937 generator = std::mt19937(rand());

                    float x = rand_x(generator);
                    float y = rand_y(generator);
                    float z = rand_z(generator);
                    float a = 0.0;

                    return imp::float4(x, y, z, a);
                };

                particle_instance_node_->AddComponent<ParticleInstances>(params);


            }).KeptBy(this);
    }

    void WaterWave::UpdateWaves()
    {
        if (sizeOfGrid_ == state_.sizeOfGrid)
            return ;
        float scale = 20.f; // resolution of wave (hard code)

        sizeOfGrid_ = state_.sizeOfGrid;
        state_.quantity = sizeOfGrid_ * sizeOfGrid_;

        float* value_data = new float[state_.quantity * 4];

        auto positionFunction = [this, scale]() {
            float size = 1.f / sizeOfGrid_ * scale * 0.5;
            std::uniform_real_distribution<float> rand_x(-size, size);
            std::uniform_real_distribution<float> rand_y(1., 1.5);
            std::uniform_real_distribution<float> rand_z(-size, size);

            std::mt19937 generator = std::mt19937(rand());

            float x = rand_x(generator);
            float y = rand_y(generator);
            float z = rand_z(generator);
            return imp::float3(x, y, z);
        };

        for(uint32_t i = 0; i < state_.quantity; ++i) {
            auto x1 = positionFunction();
            value_data[i * 4 + 0] = x1.r;
            value_data[i * 4 + 1] = x1.g;
            value_data[i * 4 + 2] = x1.b;
            value_data[i * 4 + 3] = (sin(i) + 1.0) / 2.0;
        }

        filament::Texture::PixelBufferDescriptor buffer(value_data, sizeof(float) * 4 * state_.quantity, filament::Texture::Format::RGBA,
                                                        filament::Texture::Type::FLOAT, [](void* buffer, size_t size, void* user){
            delete[] static_cast<uint32_t*>(buffer);
        });

        auto texture_ = filament::Texture::Builder()
                .width(sizeOfGrid_)
                .height(sizeOfGrid_)
                .levels(1)
                .format(filament::Texture::InternalFormat::RGBA32F)
                .build(*GetView().GetSharedEngine());
        
        texture_->setImage(*GetView().GetSharedEngine(), 0, std::move(buffer));


        auto textWrapper = GetView().GetTextureFactory().WrapTexture(texture_);
        material_ptr_->SetParameter("Texture", std::move(textWrapper));
        material_ptr_->SetParameter("TextureWidth", sizeOfGrid_);
        material_ptr_->SetParameter("Scale", scale);
    
    }

    void WaterWave::OnIsfStateChanged()
    {
        if(material_ptr_ == nullptr)
        {
            IMP_LOG(imp::ERROR) << "invalid material!";
            return;
        }
        UpdateWaves();
        material_ptr_->SetParameter("Velocity", state_.velocity);
        material_ptr_->SetParameter("Radius", state_.radius);
        material_ptr_->SetParameter("Frequency", state_.frequency);
        material_ptr_->SetParameter("Amplitude", state_.amplitude);
    }
}
