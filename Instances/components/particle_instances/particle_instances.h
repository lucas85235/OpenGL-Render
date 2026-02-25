#ifndef COMPONENTS_PARTICLE_INSTANCES_H
#define COMPONENTS_PARTICLE_INSTANCES_H

#include "imp.h"
#include "core/ncsb/component.h"
#include "third_party/filament/filament/include/filament/VertexBuffer.h"
#include "native/components/particle_instances/mesh_utils.h"
#include "native/components/particle_instances/position_utils.h"
#include "proto/components/particle_instances_state.proto.imp.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    typedef std::function<float4(float)> Function4f;

    struct App {
        filament::VertexBuffer* vb;
        filament::IndexBuffer* ib;
        filament::MaterialInstance* matInstance;
        filament::TransformManager* tm;
        utils::Entity renderable;
    };

    struct EntityInfo
    {
        float3 position;
        float3 scale = float3(1.0, 1.0, 1.0);
        float3 rotation;
    };

    struct ParticleInstancesParams {
        EntityInfo info;
        int amount;
        float3 position;
        float3 rotation;
        float3 color;
        float2 size;
        float radius;
        int angle;
        bool center;
        float2 velocity;
        MaterialPtr material;
        TexturePtr texture;
        MeshInfo* mesh;
        Function4f positionFunction = [](float index)->float4{return float4(0.0);};
    };

    class ParticleInstances : public Component
    {
    private:
        NodeHandle camera_node_;
        App app;
        utils::Entity entity_;
        MaterialPtr material;
        int particles_amount;
        int texture_width;
        int layer_mask_;
        Function4f positionFunction = [](float index)->float4{return float4(0.0);};
        EntityInfo info_;
        ParticleInstancesState state_;

    public:
        void Setup();
        void Setup(ParticleInstancesParams &params);
        void Update(const FrameTime &frame_time);
        void Cleanup();
        filament::RenderableManager::Instance GetInstance() const;
        void OnActiveStatusChanged(bool is_active);
        filament::Texture* CreateMappingTexture();
        void SetTransform(EntityInfo info);

        using IsfInfo = imp::IsfInfo<&ParticleInstances::state_>;
        void InitializeProto();
        void OnIsfStateChanged();
    };

}  // namespace ix::samsung::homecomponents

#endif // COMPONENTS_PARTICLE_INSTANCES_H
