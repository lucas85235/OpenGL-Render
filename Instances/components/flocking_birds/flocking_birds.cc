#include "native/components/flocking_birds/flocking_birds.h"
#include "native/components/particle_system/shape_utils.h"
#include "native/components/flocking_birds/flocking_birds_assets.h"

namespace ix::samsung::homecomponents {

    void FlockingBirdsManager::Setup()
    {
        InitializeFlockOfBirdsManager();
    }

    void FlockingBirdsManager::InitializeFlockOfBirdsManager()
    {
        for (int index = 0; index < state_.range.size(); index++)
        {
            FlockingBirdsData data;
            data.birds_amount   = state_.range[index].birds_amount.value_or(flockingBirds_data_.birds_amount);
            data.bird_size      = state_.range[index].bird_size.value_or(flockingBirds_data_.bird_size);
            data.initial_delay  = state_.range[index].initial_delay.value_or(flockingBirds_data_.initial_delay);
            data.show_time      = state_.range[index].show_time.value_or(flockingBirds_data_.hidden_time);
            data.hidden_time    = state_.range[index].hidden_time.value_or(flockingBirds_data_.hidden_time);
            data.velocity       = state_.range[index].velocity.value_or(flockingBirds_data_.velocity);
            data.min_z_distance = state_.range[index].min_z_distance.value_or(flockingBirds_data_.min_z_distance);
            data.max_z_distance = state_.range[index].max_z_distance.value_or(flockingBirds_data_.max_z_distance);
            data.min_y_distance = state_.range[index].min_y_distance.value_or(flockingBirds_data_.min_y_distance);
            data.max_y_distance = state_.range[index].max_y_distance.value_or(flockingBirds_data_.max_y_distance);
            data.color_1        = state_.range[index].color_1.value_or(flockingBirds_data_.color_1);
            CreateFlockOfBirdsInstances(data, index);
        }
    }

    void FlockingBirdsManager::CreateFlockOfBirdsInstances(FlockingBirdsData data, int index)
    {
        node_ = GetView().CreateNode();
        node_->SetParent(this->GetNode());

        node_->AddComponent<FlockingBirds>(data);
        node_->SetName("FlockBird_Instance: " + std::to_string(index++));
        flockingBirds_nodes_instances_.push_back(node_);
    }

    void FlockingBirds::Setup(FlockingBirdsData data)
    {
        InitializeFlockOfBirds(data);
    }

    void FlockingBirds::InitializeFlockOfBirds(FlockingBirdsData data)
    {
            flocking_birds_data_ = data;
            InitializeFlockDelayed(flocking_birds_data_.initial_delay);
    }
    void FlockingBirds::Update(const FrameTime &frame_time)
    {
        if(!is_counting_)
        {
            return;
        }

        elapsed_ += std::clamp(frame_time.GetDeltaSeconds(), 1.0f / 90.0f, 1.0f / 30.0f);
        if(elapsed_ >= duration_)
        {
            CreateFlockOfBird();
            is_counting_ = false;
        }
    }

    // Method initialize effect with delay time one once time when the effect start or is reloaded
    void FlockingBirds::InitializeFlockDelayed(float duration)
    {
        duration_ = duration;
        elapsed_ = 0;
        is_counting_ = true;
    }


    void FlockingBirds::CreateFlockOfBird() {
        auto birds_mat = GetView().GetAssetManager().LoadMaterial(assets::kFlockingBirdsMaterialCmat);
        auto bird_tex = GetView().GetAssetManager().LoadImage(assets::kTXBirdsPng);
        birds_mat.Merge(bird_tex).Then([this](std::tuple<imp::AssetPtr<imp::MaterialAsset>,
        AssetPtr<imp::ImageAsset> > result)
        {
                auto [material, image] = result;

                MaterialPtr mat = GetView().GetMaterialFactory().CreateMaterial(material);
                bird_material_ptr_ = mat.get();
                auto bird_texture = GetView().GetTextureFactory().CreateTexture(*image);

                // Setup Material Parameters
                bird_material_ptr_->SetParameter("BaseTexture", std::move(bird_texture));
                bird_material_ptr_->SetParameter("Color1", flocking_birds_data_.color_1);
                bird_material_ptr_->SetParameter("Color2", flocking_birds_data_.color_2);
                bird_material_ptr_->SetParameter("MaxFrames", flocking_birds_data_.max_frames);
                bird_material_ptr_->SetParameter("Fps", flocking_birds_data_.animation_fps);
                bird_material_ptr_->SetParameter("Tile", flocking_birds_data_.tile);

                // Setup particle system
                particle_node_ = GetView().CreateNode();
                particle_node_->SetName("FlockBirds_ParticleSystem");
                particle_node_->SetParent(this->GetNode());

                // Setup particle options
                auto options = ParticleSystemOptions();
                options.initial_particles = 1.0f;
                options.max_particles = flocking_birds_data_.birds_amount;
                options.looping = false;
                options.mesh_type = ParticleMesh::QUAD;
                options.limit_update = true;
                options.color = flocking_birds_data_.color_1;

                // Setup particle emitter
                CalculateBirdPosition();
                EmitterOptions emitter_options = InitializeEmitterOptions();

                // Particle system initialize
                auto particle_system = particle_node_->AddComponent<ParticleSystem>(options);
                particle_system->Initialize(std::move(mat));
                particle_system->AddEmitter(emitter_options, float3{3.0, 3.0, -3.0});
                particle_system->Play();
        }).KeptBy(this);
    }

    EmitterOptions FlockingBirds::InitializeEmitterOptions()
    {
        EmitterOptions emitterOptions = EmitterOptions();
        emitterOptions.emission_rate = flocking_birds_data_.hidden_time + flocking_birds_data_.show_time;
        emitterOptions.particle_life = flocking_birds_data_.show_time;
        emitterOptions.emission_quantity = flocking_birds_data_.birds_amount;
        emitterOptions.initial_particles = flocking_birds_data_.birds_amount;
        emitterOptions.is_emitter = true;

        emitterOptions.positionFunction = [this](NodeHandle) {
            auto pos = positions_[bird_pos_index_];
            bird_pos_index_++;
            // It's necessary reset index every emission rate interaction
            if (bird_pos_index_ >= flocking_birds_data_.birds_amount) {
                CalculateBirdPosition();
            }
            return float3{pos.x, pos.y, pos.z};
        };
        emitterOptions.velocityFunction = [this] {
            return flocking_birds_data_.velocity;
        };
        emitterOptions.scaleFunction = [this] {
            return float3(flocking_birds_data_.bird_size);
        };
        return emitterOptions;
    }

    // Method calculates V positions of birds
    void FlockingBirds::CalculateBirdPosition() {
        float3 pos = RandomValue(float3{50.0, flocking_birds_data_.min_y_distance, -flocking_birds_data_.min_z_distance},
                                 float3{
                                     52.0, flocking_birds_data_.max_y_distance, -flocking_birds_data_.max_z_distance
                                 }).getValue();
        float3 initialPosition_ = {pos.x, pos.y, pos.z};
        float delta = 0.05f;
        float2 step = flocking_birds_data_.quad_max_size / flocking_birds_data_.birds_amount;
        auto halfCount = flocking_birds_data_.birds_amount / 2;

        positions_.clear();
        positions_.push_back(initialPosition_);
        for (int i = 1; i < flocking_birds_data_.birds_amount; i++) {
            auto randomStep = RandomValue(step - delta, step + delta).getValue();
            if (i <= halfCount)
                positions_.push_back(positions_[i - 1] + float3(-randomStep.x, randomStep.y / 1.5, 0.0));
            else
                positions_.push_back(positions_[i - 1] + float3(randomStep.x * 2.0, randomStep.y / 3.0, 0.0));
        }
        bird_pos_index_ = 0;
    }

#if IMP_RUNTIME(DEV)
    void FlockingBirdsManager::DrawEditorUi()
    {
        if (ImGui::Button("Reload effect"))
        {
            for(auto nodes : flockingBirds_nodes_instances_)
            {
                GetView().DestroyNode(nodes);
            }
            flockingBirds_nodes_instances_.clear();
            InitializeFlockOfBirdsManager();
        }
    }
#endif // IMP_RUNTIME(DEV)
}
