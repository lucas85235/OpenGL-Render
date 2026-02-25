#pragma once

#include "imp.h"
#include "native/components/particle_system/particle_system.h"
#include "native/components/fireflies_effect/fireflies_assets.h"
#include "native/home_components_utils/common/math/math.h"
#include "core/ncsb/component.h"

namespace ix::samsung::homecomponents {
    struct SpawnProperties{
        // number of firefly particles that are spawned
        int number_of_fireflies = 1000;
        // the maximum distance that the fireflies will move to
        imp::float3 max_movement_range = imp::float3(23.0f,1.0f,2.0f);
        // the maximum alpha when a firefly blinks
        float max_brightness = 1.0;
        //used to change the effect position randomly by time
        bool change_position_by_time = true;
        //time to change the effect position to a random one
        float time_to_change_position = 40.0f;
        //range that is used to choose randomly the new position
        float random_position_radius_area = 20.0f;
        //effect center to make the circle
        imp::float3 effect_center = imp::float3(0.0f,0.0f,0.0f);
        // the min time that a firefly can blink randomly
        float min_random_time_to_blink = 1.0;
        // the max time that a firefly can blink randomly
        float max_random_time_to_blink = 1.0;
        // the firefly max to use in a random color selection when at the maximum alpha
        imp::float3 firefly_max_base_color = imp::float3(1.0f, 1.0f, 0.639f);
        // the firefly min to use in a random color selection when at the maximum alpha
        imp::float3 firefly_min_base_color = imp::float3(1.0f, 1.0f, 0.639f);
        // the firefly color when at the minimum alpha
        imp::float3 firefly_min_color = imp::float3(1.0f, 1.0f, 0.0f);
        // the final velocity multiplier for each firefly 
        float velocity_multiplier = 1.0;
        //size multiplier to change the fireflies size
        float shader_size_multiplier = 0.4;
        ParticleMesh mesh_type = ParticleMesh::QUAD;
    };

    class Fireflies : public imp::Component
    {
        public:

        /**
         * Setup with properties that are used to tell the component all the characteristics about it's behavior
         * and appearance
         */
        void Setup(SpawnProperties properties);
        void Setup();

        private:

        float current_time_;
        SpawnProperties properties_;
        std::mt19937 generator_;
        //used to change the effect position randomly
        void ChangePosition();
        float RandomNumber(float min, float max);
    };

} // namespace ix::samsung::homecomponents
