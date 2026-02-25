#pragma once

#include "imp.h"
#include <random>

namespace ix::samsung::homecomponents
{
    class Shape
    {
    private:
        std::function<imp::float3(imp::NodeHandle node)> shapeFunction = [this](imp::NodeHandle node){ return build(node); };
    public:
        Shape(){};

        // method to build the shape
        virtual imp::float3 build(imp::NodeHandle) = 0;

        std::function<imp::float3(imp::NodeHandle)> get(){ return this->shapeFunction;};

    };

    class SphereShape : public Shape
    {
    private:
        float radius_;
        imp::float2 radial_;
        imp::float2 angular_;
    public:
        SphereShape(float radius, imp::float2 radial, imp::float2 angular);

        imp::float3 build(imp::NodeHandle);
    };

    class CircleShape : public Shape
    {
    private:
        float min_radius_;
        float max_radius_;
    public:
        CircleShape(float max_radius);
        CircleShape(float min_radius, float max_radius);

        imp::float3 build(imp::NodeHandle);
    };

    class TopCircleShape : public Shape
    {
    private:
        float radius_;
    public:
        TopCircleShape(float radius);

        imp::float3 build(imp::NodeHandle);
    };
}
