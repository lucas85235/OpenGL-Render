#ifndef COMPONENTS_CLOUDS_H
#define COMPONENTS_CLOUDS_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "proto/components/moving_clouds_state.proto.imp.h"
#include "core/view/framework/render/material.h"
#include "MeshUtils.h"

namespace ix::samsung::homecomponents
{
    struct MovingCloudParams {
        // Material parameters, with default values
        float kDensity = 1.1f;
        float kVelocity = 0.6f;
        float kSamples = 2.0f;
        float kSmallCloudsSpeed = 0.2f;
        imp::float4 kColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		float tiling = 1.5f;
	};

    class MovingCloudsComponent : public imp::Component
    {
    private:
		std::unique_ptr<MeshUtils> meshUtils;
		MovingCloudsState state_;
        imp::MaterialPtr material_ptr_;
        imp::ComponentHandle<imp::RenderComponent> renderer_;
		imp::ComponentHandle<MeshUtils> meshutilsComponent;

    public:
        using IsfInfo = imp::IsfInfo<&MovingCloudsComponent::state_>;

        void Setup();
        void Setup(MovingCloudParams& params);
        void InitializeClouds();
        void OnIsfStateChanged();
        void VisitNodeTreeToDisableGltfColliders(imp::NodeHandle node);
		void CreateQuads(imp::float2 pos);
    };
}

#endif // COMPONENTS_CLOUDS_H
