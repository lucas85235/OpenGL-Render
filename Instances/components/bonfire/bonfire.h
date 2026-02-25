#ifndef COMPONENTS_BONFIRE_H
#define COMPONENTS_BONFIRE_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "proto/components/bonfire_state.proto.imp.h"
#include "native/components/particle_system/particle_system.h"

namespace ix::samsung::homecomponents
{
    class Bonfire : public imp::Component
    {
    private:
        imp::NodeHandle flames_node_;
        imp::NodeHandle base_node_;
        imp::NodeHandle sparks_node_;
        imp::NodeHandle bonfire_sprite_node_;
        imp::NodeHandle ground_light_sprite_node_;
        BonfireState state_;
        float RandomNumber(float min, float max);

    public:
        void Setup();
        using IsfInfo = imp::IsfInfo<&Bonfire::state_>;

        void Initialize();
        void AddBonfireSprite();
        void AddGroundLightSprite();
        void AddFlames();
        void AddFlame(const std::string &node_name, imp::float3 local_pos, FlameState state, int priority);
        void AddBase(imp::float3 local_pos, BonfireBaseState state);
        void AddSparks(SparksState state);

#if IMP_RUNTIME(DEV)
        void DrawEditorUi();
#endif

    };
}

#endif // COMPONENTS_BONFIRE_H
