#pragma once

#include "imp.h"
#include <random>
#include <cmath>

namespace ix::samsung::homecomponents {
    class Position {
    private:
        std::function<imp::float4(float)> shapeFunction = [this](float index_) { return build(index_); };

    public:
        Position() {
        };

        // method to build the shape
        virtual imp::float4 build(float index_) = 0;

        std::function<imp::float4(float)> get() { return this->shapeFunction; };
    };

    class SpherePosition : public Position {
    private:
        float radius_;
        imp::float2 radial_;
        imp::float2 angular_;

    public:
        SpherePosition(float radius, imp::float2 radial, imp::float2 angular): radius_(radius), radial_(radial), angular_(angular) {
        };

        imp::float4 build(float index_) {
            std::uniform_real_distribution<float> rand_radial(radial_[0], radial_[1]);
            std::uniform_real_distribution<float> rand_angular(angular_[0], angular_[1]);
            std::uniform_real_distribution<float> random(0.0, 1.0);

            std::random_device generator = std::random_device();

            float radial_coordinate = rand_radial(generator);
            float angular_coordinate = rand_angular(generator);
            float random_coordinate = random(generator);

            float theta = radial_coordinate * 2.0 * M_PI;
            float phi = acos(1.0 - 2.0 * angular_coordinate);

            imp::float4 position;
            position.x = radius_ * cos(theta) * sin(phi);
            position.y = radius_ * cos(phi);
            position.z = radius_ * sin(theta) * sin(phi);

            position.x = (position.x + 1.0) / 2.0;
            position.y = (position.y + 1.0) / 2.0;
            position.z = (position.z + 1.0) / 2.0;
            position.a = 0.0;

            // output::Warning("%f, %f %f", position.x, position.y, position.z);
            return {position};
        }
    };

    class RingPosition : public Position    {
    private:
        float radius_, ring_;

    public:
        RingPosition(float radius, float ring): radius_(radius), ring_(ring){};
        imp::float4 build(float index_) {
            std::uniform_real_distribution<float> randr(ring_, 1.0);
            std::uniform_real_distribution<float> randh(0.0, 1.0);
            std::random_device generator = std::random_device();

            float radial_coordinate = randh(generator);
            float rand_radius = randr(generator);
            float rand_h = randh(generator);

            float theta = radial_coordinate * 2.0 * filament::math::f::PI;
            float r = radius_ * sqrt(rand_radius);

            imp::float4 position;
            position.x = r * cos(theta);
            position.y = rand_h * 1.0;
            position.z = r * sin(theta);

            position.x = (position.x + 1.0) / 2.0;
            position.y = (position.y + 1.0) / 2.0;
            position.z = (position.z + 1.0) / 2.0;
            position.a = 0.0;

            return {position};
        }
    };

    class RingLookAtPosition : public Position    {
    private:
        float radius_, ring_;

    public:
        RingLookAtPosition(float radius, float ring): radius_(radius), ring_(ring){};
        imp::float4 build(float index_) {
            std::uniform_real_distribution<float> randr(ring_, 1.0);
            std::uniform_real_distribution<float> randh(0.0, 1.0);
            std::random_device generator = std::random_device();

            float radial_coordinate = randh(generator);
            float rand_radius = randr(generator);
            float rand_h = randh(generator);

            float theta = radial_coordinate * 2.0 * filament::math::f::PI;
            float r = radius_ * sqrt(rand_radius);

            imp::float4 position;
            position.x = r * cos(theta);
            position.y = rand_h;
            position.z = r * sin(theta);

            position.x = (position.x + 1.0) / 2.0;
            position.y = (position.y + 1.0) / 2.0;
            position.z = (position.z + 1.0) / 2.0;

            float angle = M_PI / 2.0 - theta;
            position.a = angle;

            return {position};
        }
    };

    class CirclePosition : public Position
    {
    private:
        float radius_;
        imp::float3 center_;
        imp::float2 rand = imp::float2(0.0, 1.0);

    public:
        CirclePosition(float radius): radius_(radius) {
        };

        imp::float4 build(float index_) {
            std::uniform_real_distribution<float> rand(0.0, 1.0);
            std::random_device generator = std::random_device();

            float radial_coordinate = rand(generator);
            float rand_radius = rand(generator);

            float theta = radial_coordinate * 2.0 * filament::math::f::PI;
            float r = radius_ * sqrt(rand_radius);

            imp::float4 position;
            position.x = r * cos(theta);
            position.z = r * sin(theta);

            position.x = (position.x + 1.0) / 2.0;
            position.z = (position.z + 1.0) / 2.0;
            position.a = 0.0;


            return {position};
        }
    };

    class CirclePositionStars : public Position    {
    private:
        float radius_, radial_;

    public:
        CirclePositionStars(float radius, float radial): radius_(radius), radial_(radial){};
        imp::float4 build(float index_) {
            std::uniform_real_distribution<float> randr(0.0, 1.0);
            std::uniform_real_distribution<float> randh(0.0, radial_);
            std::random_device generator = std::random_device();

            float radial_coordinate = randh(generator);
            float rand_radius = randr(generator);
            float rand_h = randh(generator);

            float theta = radial_coordinate * 2.0 * filament::math::f::PI;
            float r = radius_ * sqrt(rand_radius);

            imp::float4 position;
            position.x = r * cos(theta);
            position.y = r * sin(theta);
            position.z = 0.0;

            position.x = (position.x + 1.0) / 2.0;
            position.y = (position.y + 1.0) / 2.0;
            position.z = (position.z + 1.0) / 2.0;
            position.a = 0.0;

            return {position};
        }
    };

    class ConePosition: public Position
    {
    private:
        float min_radius_;
        float max_radius_;

    public:
        ConePosition(float minradius, float maxradius)
            :min_radius_(minradius), max_radius_(maxradius) {}

        imp::float4 build(float index_)
        {
            std::uniform_real_distribution<float> rand_radius(min_radius_, max_radius_);
            std::uniform_real_distribution<float> rand_theta(1.0, 20.0 * M_PI);
            std::uniform_real_distribution<float> height(0.0, 1.0);


            std::mt19937 generator = std::mt19937(rand());

            float radius = rand_radius(generator);
            float theta = rand_theta(generator);
            float h = height(generator);
            float maxHeight = 20.0;

            //float angle = M_PI/2 - theta;

            imp::float4 position;
            position.x = (cos(theta) * radius);
            position.y = maxHeight * h;
            position.z = (sin(theta) * radius);
            position.a = 0.0;

            //imp::output::Info("%f, %f %f", position.x, position.y, position.z);

            return {position};
        }
    };

    class ConeV4Angle: public Position
    {
    private:
        float min_radius_;
        float max_radius_;
        float max_height_;
        float radian_angle_;

    public:
        ConeV4Angle(float minRadius, float maxRadius, float max_height, float radianAngle)
            :min_radius_(minRadius), max_radius_(maxRadius), max_height_(max_height), radian_angle_(radianAngle) {}

        imp::float4 build(float index_)
        {
            std::uniform_real_distribution<float> rand_radius(min_radius_, max_radius_);
            std::uniform_real_distribution<float> rand_theta(1.0, 20.0 * M_PI);
            std::uniform_real_distribution<float> height(0.0, 1.0);


            std::mt19937 generator = std::mt19937(rand());

            float radius = rand_radius(generator);
            float theta = rand_theta(generator);
            float h = height(generator);
            float maxHeight = max_height_;

            float angle = M_PI/2 - theta;

            imp::float4 position;
            position.x = (cos(theta) * radius);
            position.y = maxHeight * h;
            position.z = (sin(theta) * radius);
            if(radian_angle_ != 0.0)
                position.a = radian_angle_;
            else
                position.a = angle;
            //imp::output::Info("%f, %f %f", position.x, position.y, position.z);

            return position;
        }
    };

    class LinePosition : public Position  {
    private:
        float size_;

    public:
        LinePosition(float size): size_(size){};
        imp::float4 build(float index_) {
            std::uniform_real_distribution<float> line(0.0, 1.0);
            std::random_device generator = std::random_device();

            float line_coordinate = line(generator);

            imp::float4 position;
            position.x = line_coordinate * size_;
            position.y = 0.0;
            position.z = 0.0;

            position.x = (position.x + 1.0) / 2.0;
            position.y = (position.y + 1.0) / 2.0;
            position.z = (position.z + 1.0) / 2.0;
            position.a = 0.0;

            return {position};
        }
    };

    class LinePositionSequential : public Position {

    public:
        LinePositionSequential() : Position() {}
        imp::float4 build(float index_) {

            float y = imp::mix(0.0, 1.0, index_);

            imp::float4 position;
            position.x = 0.0;
            position.y = y;
            position.z = 0.0;

            position.x = (position.x + 1.0) / 2.0;
            position.y = (position.y + 1.0) / 2.0;
            position.z = (position.z + 1.0) / 2.0;
            position.a = 0.0;

            return {position};
        }
    };

    class PlanePosition : public Position    {
    private:
        float size_;

    public:
        PlanePosition(float size): size_(size){};
        imp::float4 build(float index_) {
            std::uniform_real_distribution<float> randX(-1.0, 1.0);
            std::uniform_real_distribution<float> randZ(-1.0, 1.0);
            std::random_device generator = std::random_device();

            imp::float3 coordinate = imp::float3(randX(generator), 0.0, randZ(generator));

            imp::float4 position;
            position.x = coordinate.x * size_;
            position.y = 0.0;
            position.z = coordinate.z * size_;

            position.x = (position.x + 1.0) / 2.0;
            position.y = (position.y + 1.0) / 2.0;
            position.z = (position.z + 1.0) / 2.0;
            position.a = 0.0;

            return {position};
        }
    };
}
