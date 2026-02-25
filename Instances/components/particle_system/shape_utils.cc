#include "shape_utils.h"

namespace ix::samsung::homecomponents
{
    // Circle Shape

    SphereShape::SphereShape(float radius, imp::float2 radial, imp::float2 angular): radius_(radius), radial_(radial), angular_(angular){};

    imp::float3 SphereShape::build(imp::NodeHandle node){
        std::uniform_real_distribution<float> rand_radial(radial_[0], radial_[1]);
        std::uniform_real_distribution<float> rand_angular(angular_[0], angular_[1]);

        std::mt19937 generator = std::mt19937(rand());

        float radial_coordinate = rand_radial(generator);
        float angular_coordinate = rand_angular(generator);

        float theta = radial_coordinate * 2.0 * M_PI;
        float phi = acos(2.0 * angular_coordinate - 1.0);

        return imp::float3{
            radius_ * cos(theta) * sin(phi),
            radius_ * cos(phi),
            radius_ * sin(theta) * sin(phi)
        };
    }

    // Circle Shape

    CircleShape::CircleShape(float max_radius): min_radius_(0.0f), max_radius_(max_radius){};

    CircleShape::CircleShape(float min_radius, float max_radius): min_radius_(min_radius), max_radius_(max_radius){};

    imp::float3 CircleShape::build(imp::NodeHandle node){
        std::uniform_real_distribution<float> rand_radius(min_radius_, max_radius_);
        std::uniform_real_distribution<float> rand_theta(0.0, 2.0 * M_PI);

        std::mt19937 generator = std::mt19937(rand());

        float radius = rand_radius(generator);
        float theta = rand_theta(generator);

        auto rotation = imp::quatf::fromAxisAngle(imp::kUp, (M_PI/2 - theta));
        node->SetLocalRotation(rotation);

        return imp::float3{
            cos(theta) * radius,
            0,
            sin(theta) * radius
        };
    }

    TopCircleShape::TopCircleShape(float radius): radius_(radius){};

    imp::float3 TopCircleShape::build(imp::NodeHandle node){
        std::uniform_real_distribution<float> rand_radius(0, radius_);
        std::uniform_real_distribution<float> rand_theta(0.0, 2.0 * M_PI);

        std::mt19937 generator = std::mt19937(rand());

        float radius = rand_radius(generator);
        float theta = rand_theta(generator);

        auto rotation = imp::quatf::fromAxisAngle(imp::kRight, (M_PI/2));
        node->SetLocalRotation(rotation);

        return imp::float3{
            cos(theta) * radius,
            0,
            sin(theta) * radius
        };
    }
}
