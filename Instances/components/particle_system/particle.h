#pragma once

#include "imp.h"

namespace ix::samsung::homecomponents {

    // Struct for a particle
    struct Particle {
        bool operator == (const Particle& p) const { return node == p.node; } // needed for remove() on std::list
        Particle() {};
        Particle(imp::float3 position, imp::float3 velocity, imp::float3 acceleration, imp::float3 rotation, imp::float4 rgba, float scale, float amplitude, float period) {
            this->position = position;
            this->velocity = velocity;
            this->acceleration = acceleration;
            this->rotation = rotation;
            this->rgba = rgba;
            this->scale = scale;
            this->amplitude = amplitude;
            this->period = period;
        }

        imp::Material* material;                    // Material instance
        imp::float3 position;                       // Position
        imp::float3 velocity;                       // Velocity
        imp::float3 acceleration;                   // Acceleration
        imp::float3 rotation;                       // rotation
        imp::float4 rgba;                           // Color and alpha
        imp::float3 scale;                          // Object Scale
        float amplitude;                            // Amplitude
        float period;                               // Period
        bool loop;                                  // In loop

        imp::NodeHandle node;                       // Object reference
        float life;                                 // Particle lifetime
        float current_life;                         // Current Particle life
        int pool_index;                             // Used when returning the object to the pool
    };
}
