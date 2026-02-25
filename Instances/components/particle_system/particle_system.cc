#include "native/components/particle_system/particle_system.h"

#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>
#include "particle_system.h"

namespace ix::samsung::homecomponents {

    // ----- Impress/Setup Component Methods ----- //

    void ParticleSystem::Setup(ParticleSystemOptions options) {
        std::random_device rand;
        generator = std::mt19937(rand());

        options_ = options;

        mesh_ = CreateParticleMesh(options_.mesh_type);
    }

    void ParticleSystem::Update(const imp::FrameTime& frame_time) {

        if(!playing_) return;

        count_update_ += std::clamp(frame_time.GetDeltaSeconds(),1.0f/90.0f,1.0f/30.0f);

        // Update can be a fixed update called about 18 times by second
        if (options_.limit_update && count_update_ <= FIXED_TIME_) {
            return;
        }

        // Update particle system, using delta time
        ParticleUpdate(count_update_);

        // Reset time count
        count_update_ = 0;
    }

    // ----- Particle System Methods ----- //

    void ParticleSystem::Initialize(imp::MaterialPtr material) {
        playing_ = false;
        material_ = std::move(material);
        FillParticlePool();
    }

    void ParticleSystem::FillParticlePool() {

        particle_pool_.reserve(options_.max_particles);
        for (int i = 0; i < options_.max_particles; ++i)
        {
            ParticleCreate();
        }
    }

    void ParticleSystem::Play() {
        for(auto emitter: emitters) {
            emitter->EmitInitialParticles();
        }
        playing_ = true;
    }

    void ParticleSystem::Pause() {
        playing_ = false;
    }

    void ParticleSystem::Stop() {
        playing_ = false;
    }

    void ParticleSystem::PauseEmission()
    {
        for (const auto emitter: emitters)
        {
            emitter->emitterOptions_.is_emitter = false;
        }
    }

    void ParticleSystem::StartEmission()
    {
        for (const auto emitter: emitters)
        {
            emitter->emitterOptions_.is_emitter = true;
        }
    }

    // todo: isso precisa ser feito de fora do shader
    void ParticleSystem::ParticleCreate() {
        std::shared_ptr<Particle> particle = std::make_shared<Particle>();
        particle->node = CreateShapeNode(particle.get());
        particle->node->SetEnabled(false);

        // Add the created particle to the pool, and store its index into the queue
        particle_pool_.emplace_back(particle);
        particle_indexes_.push(particle_pool_.size() - 1);
    }

    void ParticleSystem::ParticleUpdate(float delta_time)
    {
        // Particle life cycle
        bool has_disabled_particles = false;

        for (const auto& particle : particles_) {

            particle->material->TrySetParameter("CurrentTime", particle->current_life);
            if (particle->current_life > particle->life)
            {
                if (options_.looping)
                {
                    particle->current_life = 0.0f;
                }
                else
                {
                    particle->node->SetEnabled(false);
                    particle_indexes_.push(particle->pool_index);
                    has_disabled_particles = true;
                }
            }
            else
            {
                particle->current_life += delta_time;
            }
        }

        // Remove all particles with disabled nodes from the list
        if (has_disabled_particles)
        {
            particles_.remove_if([](std::shared_ptr<Particle> &p) {
                return !p->node->IsEnabled();
            });
        }
    }

    void ParticleSystem::AddEmitter(EmitterOptions e, imp::float3 position, imp::quatf rotation)
    {
        auto node = GetView().CreateNode();
        node->SetName("Emitter");
        node->SetParent(GetNode());
        node->SetLocalPosition(position);
        node->SetLocalRotation(rotation);
        auto emitter = node->AddComponent<Emitter>(e, this);
        emitters.push_back(emitter.Get());
    }

    
    void ParticleSystem::ApplyParametersToShader(Particle* particle)
    {
        particle->material->TrySetParameter("Color", particle->rgba);
        particle->material->TrySetParameter("Life", particle->life);
        particle->material->TrySetParameter("Velocity", particle->velocity);
        particle->material->TrySetParameter("Rotation", particle->rotation);
        particle->material->TrySetParameter("Acceleration", particle->acceleration);
        particle->material->TrySetParameter("Noise", options_.noise.getValue());
        particle->material->TrySetParameter("SizeMultiplier", options_.shader_size_multiplier);
        particle->material->TrySetParameter("CurrentTime", 0.0f);
        particle->material->TrySetParameter("Amplitude", particle->amplitude);
        particle->material->TrySetParameter("Period", particle->period);
        particle->material->TrySetParameter("Index", particle->pool_index);
    }

    bool ParticleSystem::IsPlaying()
    {
        return playing_;
    }

    std::shared_ptr<Particle> ParticleSystem::GetAvailableParticle()
    {
        int pool_index = particle_indexes_.front();
        particle_indexes_.pop();
        // Retrieve a Particle instance from the pool
        std::shared_ptr<Particle> particle = particle_pool_[pool_index];
        if (particle->node->IsEnabled()) {
            imp::output::Error("!!!!!!!!!!!!!!!!!PARTICLE ENABLED ON THE PARTICLE INDEXES!!!!!!!!!!!!!!");
            return nullptr;
        }
        // Store the pool index for this particle, so it can be properly returned to the pool when disabled
        particle->pool_index = pool_index;

        // Particle initialization
        particle->rgba = options_.color.getValue();
        particle->amplitude = CalculateAmplitude();
        particle->period = CalculatePeriod();

        particle->life = options_.particle_life;
        particle->current_life = 0.0;
        particle->loop = options_.looping;

        particle->node->SetEnabled(true);
        particles_.push_back(particle);

        return particle;
    }

    int ParticleSystem::SizeAvailableParticles()
    {
        return particle_indexes_.size();
    }

    // ----- Utils Methods ----- //

    imp::NodeHandle ParticleSystem::CreateShapeNode(Particle* particle) {
        imp::NodeHandle shape = GetView().CreateNode();
        shape->SetName("Particle");
        shape->SetParent(GetNode());
        imp::ComponentHandle<imp::RenderComponent> shape_renderer =
                shape->AddComponent<imp::RenderComponent>(
                        imp::RenderComponent::FrustrumCullingMode::kDisabled);
        shape_renderer->SetShadowCastingMode(imp::RenderComponent::ShadowMode::kNone);
        shape_renderer->SetShadowReceivingMode(imp::RenderComponent::ShadowMode::kNone);
        shape_renderer->SetBlendOrder(static_cast<uint16_t>(imp::RenderComponent::BlendOrderMode::kGlobal));
        shape_renderer->SetChannel(3);
        shape_renderer->SetPriority(options_.render_priority);
        shape_renderer->SetFogEnabled(false);

        // Setup mesh or custom mesh
        if (options_.mesh_type == ParticleMesh::CUSTOM && custom_mesh_ == nullptr) {
            imp::output::Info("Custom mesh is null, Default mesh has been settled");
        } // don't use else if
        if (options_.mesh_type == ParticleMesh::CUSTOM && custom_mesh_ != nullptr) {
            shape_renderer->SetMesh(custom_mesh_.get());
        }
        else shape_renderer->SetMesh(mesh_.get());

        // Duplicate material
        filament::MaterialInstance* material_instance = filament::MaterialInstance::duplicate(material_->GetFilamentMaterialInstance());
        imp::MaterialPtr particle_ptr = GetView().GetMaterialFactory().WrapMaterial(std::move(material_instance));
        shape_renderer->SetMaterial(std::move(particle_ptr));
        particle->material = shape_renderer->GetMaterial(0);

        return shape;
    }

    // ----- Private Methods ----- //

    imp::MeshPtr ParticleSystem::CreateParticleMesh(ParticleMesh mesh_type) {
        switch (mesh_type) {
            case ParticleMesh::BOX:
                return std::move(GetView().GetMeshFactory().CreateBox({
                    .size = imp::float3(1.0f, 1.0f, 1.0f)}));
            case ParticleMesh::SPHERE:
                return std::move(GetView().GetMeshFactory().CreateSphere({
                    .radius = 1.0f}));
            default:
                return std::move(GetView().GetMeshFactory().CreateQuad({
                    .size = imp::float2(1.0f, 1.0f)}));
        };
    }

    imp::float3 ParticleSystem::CalculatePositionShapes(PositionShapes type, imp::NodeHandle node) {
        switch (type) {
            case PositionShapes::DEFAULT: {
                return options_.position.getValue();
            }
            case PositionShapes::SPHERE: {
                std::uniform_real_distribution<float> rand_radial(0.0, 1.0);
                std::uniform_real_distribution<float> rand_angular(0.0, 1.0);

                std::mt19937 generator = std::mt19937(rand());

                float radial_coordinate = rand_radial(generator);
                float angular_coordinate = rand_angular(generator);

                float theta = radial_coordinate * 2.0 * M_PI;
                float phi = acos(2.0 * angular_coordinate + 0.25);

                return imp::float3{
                    options_.position.getValue().x * cos(theta) * sin(phi),
                    options_.position.getValue().y * cos(phi),
                    options_.position.getValue().z * sin(theta) * sin(phi)
                };
            }
            case PositionShapes::CIRCLE: {
            
                std::uniform_real_distribution<float> rand_theta(0.0, 2.0 * M_PI);
 
                std::mt19937 generator = std::mt19937(rand());
 
                float radius = options_.position.getValue().x;
                float theta = rand_theta(generator);
               
                auto rotation = imp::quatf::fromAxisAngle(imp::kUp, (M_PI/2 - theta));
                node->SetLocalRotation(rotation);

                return imp::float3{
                    cos(theta) * radius,
                    options_.position.getValue().y,
                    sin(theta) * radius
                };
            }
            case PositionShapes::UP_CIRCLE: {
                 std::uniform_real_distribution<float> rand_theta(0.0, 2.0 * M_PI);
 
                std::mt19937 generator = std::mt19937(rand());
 
                float radius = options_.position.getValue().x;
                float theta = rand_theta(generator);
               
                auto rotation = imp::quatf::fromAxisAngle(imp::kRight, (M_PI/2));
                node->SetLocalRotation(rotation);

                return imp::float3{
                    cos(theta) * radius,
                    options_.position.getValue().y,
                    sin(theta) * radius
                };
            }
        };
    }

    imp::float3 ParticleSystem::CalculateVelocity(VelocityType type) {
        switch (type) {
            case VelocityType::DEFAULT:
                return options_.velocity.getValue();
            case VelocityType::COS_SIN_COS:
                return imp::float3{
                    std::cos(options_.velocity.getValue().x),
                    std::sin(options_.velocity.getValue().y),
                    std::cos(options_.velocity.getValue().z)};
        };
    }

    imp::float3 ParticleSystem::CalculateRotation() {
        return imp::float3{
                std::cos(options_.rotation.getValue().x),
                std::sin(options_.rotation.getValue().y),
                std::cos(options_.rotation.getValue().z)};
    }

    imp::float3 ParticleSystem::CalculateAcceleration() {
        return imp::float3{
                options_.acceleration.getValue().x,
                options_.acceleration.getValue().y,
                options_.acceleration.getValue().z
        };
    }

    imp::float3 ParticleSystem::CalculateScale(ScaleType type) {
        switch (type) {
            case ScaleType::DEFAULT:
                return options_.scale.getValue();
            case ScaleType::UNIFORM:
                auto scale = options_.scale.getValue().x;
                return imp::float3{scale};
        };
    }

    float ParticleSystem::CalculateAmplitude() {
        return options_.amplitude.getValue();
    }

    float ParticleSystem::CalculatePeriod() {
        return options_.period.getValue();
    }

    // ----- Public Methods ----- //

    void ParticleSystem::SetCustomMesh(imp::MeshPtr custom_mesh) {
        custom_mesh_ = std::move(custom_mesh);
    }

    void ParticleSystem::DestroyAllParticles() {
        GetView().DestroyNode(GetNode());
    }
}
