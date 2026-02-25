#ifndef COMPONENTS_FLOCKING_BIRDS_H
#define COMPONENTS_FLOCKING_BIRDS_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "proto/components/flocking_birds_state.proto.imp.h"
#include "native/components/particle_system/particle_system.h"


using namespace imp;
namespace ix::samsung::homecomponents
{
    struct FlockingBirdsData
    {
        // Particle System Parameters
        float  birds_amount   = 11;
        float  bird_size      = 0.5f;
        float  initial_delay  = 0.0f;
        float  show_time      = 65.0f;
        float  hidden_time    = 10.0f;
        float  velocity       = 3.2f;
        float  min_z_distance = 57.8;
        float  max_z_distance = 61.8;
        float  min_y_distance = 3.5;
        float  max_y_distance = 4.5;
        float4 color_1        = { 0.0, 0.0, 0.0, 1.0 };

        // Material Parameters
        float animation_fps   = 60.0f;
        float2 quad_max_size  = {5.5f, 2.6f};
        float4 color_2        = { 1.0, 1.0, 1.0, 1.0 };
        float2 tile           = {8.0, 8.0};
        float  max_frames     = 64.0;
    };

    class FlockingBirdsManager : public Component
    {
        NodeHandle node_;
        FlockingBirdsState state_;
        FlockingBirdsData flockingBirds_data_;
        std::vector<NodeHandle> flockingBirds_nodes_instances_;;

    public:
        void Setup();
        void InitializeFlockOfBirdsManager();
        void CreateFlockOfBirdsInstances(FlockingBirdsData data, int index);
        using IsfInfo = IsfInfo<&FlockingBirdsManager::state_>;

#if IMP_RUNTIME(DEV)
        void DrawEditorUi();
#endif
    };

    class FlockingBirds : public Component
    {
        // Global variables
        NodeHandle particle_node_;
        Material *bird_material_ptr_;
        FlockingBirdsData flocking_birds_data_;
        std::vector<NodeHandle> particle_instance_nodes_;

        // Counter variables
        bool is_counting_ = false;
        float duration_;
        float elapsed_;

        // Variable aux to control bird position
        int bird_pos_index_ = 0;

    public:
        std::vector<float3> positions_;
        void Setup(FlockingBirdsData data);
        void InitializeFlockOfBirds(FlockingBirdsData data);
        void Update(const FrameTime &frame_time);
        void CreateFlockOfBird();
        void CalculateBirdPosition();
        void InitializeFlockDelayed(float delay);
        void RunCoroutine();
        EmitterOptions InitializeEmitterOptions();
    };
}
#endif // COMPONENTS_FLOCKING_BIRDS_H