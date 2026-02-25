#include "native/components/hand/hand_renderer.h"
#include "absl/strings/string_view.h"
#include "proto/components/hand_renderer_state.proto.imp.h"
#include "native/data/home_components_assets.h"
#include "core/xr/openxr_events.h"

#if IMP_PLATFORM(ANDROID)
#include "native/openxr/hand_tracking/handtracking_actions.h"
#endif

namespace ix::samsung::homecomponents {

using MaterialPtr = imp::AssetPtr<imp::MaterialAsset>;
using GltfPtr = imp::AssetPtr<imp::GltfAsset>;

constexpr absl::string_view kJointNames[HandRenderer::kNumJoints] = {"Palm",
                                             "Wrist",
                                             "ThumbMetacarpal",
                                             "ThumbProximal",
                                             "ThumbDistal",
                                             "ThumbTip",
                                             "IndexMetacarpal",
                                             "IndexProximal",
                                             "IndexIntermediate",
                                             "IndexDistal",
                                             "IndexTip",
                                             "MiddleMetacarpal",
                                             "MiddleProximal",
                                             "MiddleIntermediate",
                                             "MiddleDistal",
                                             "MiddleTip",
                                             "RingMetacarpal",
                                             "RingProximal",
                                             "RingIntermediate",
                                             "RingDistal",
                                             "RingTip",
                                             "LittleMetacarpal",
                                             "LittleProximal",
                                             "LittleIntermediate",
                                             "LittleDistal",
                                             "LittleTip"};

std::string GetJointNodeName(const HandRenderer::Handedness handedness, const int joint_index) {
    return absl::StrCat(handedness == HandRenderer::Handedness::LEFT ? "L_" : "R_",
                        kJointNames[joint_index]);
}

imp::resources::ResourceDefinition GetHandGlb(const HandRenderer::Handedness handedness) {
    return handedness == HandRenderer::Handedness::LEFT ? assets::kGooglehandLGlb
                                                        : assets::kGooglehandRGlb;
}

void HandRenderer::Setup(const Handedness handedness) {
    handType_ = handedness;

    GetView().GetAssetManager().LoadModel(GetHandGlb(handedness))
            .Then([this, handedness](imp::NodeHandle nodeHand)
            {
                nodeHand->SetName("Hands");
                nodeHand->SetParent(this->GetNode());

                // Construct GLTFs.
                auto gltfRendererHand = nodeHand->GetComponent<imp::GltfRenderer>();
                hand_mask_gltf_ = gltfRendererHand;
                //outline_gltf_ = gltfRendererHand;

                // Construct bone joints.
                for (int i = 0; i < kNumJoints; i++) {
                    const std::string joint_name = GetJointNodeName(handedness, i);

                    imp::NodeHandle hand_mask_joint = hand_mask_gltf_->GetOrCreateNode(joint_name);
                    //imp::NodeHandle outline_joint = outline_gltf_->GetOrCreateNode(joint_name);

                    if (!hand_mask_joint) { /*|| !outline_joint*/
                        imp::output::Warning("Hand GLTF mesh has no %s bone defined.", joint_name);
                    }else{
                        imp::output::Info(":D Hand GLTF mesh has %s.", joint_name);
                    }

                    hand_mask_joints_[i] = hand_mask_joint;
                    //outline_joints_[i] = outline_joint;
                }
            }).KeptBy(this);
}

void HandRenderer::UpdateHandJoints(const HandActions& hand){

    // Position bone joints.
    for (int joint_idx = 0; joint_idx < hand.joints().size(); ++joint_idx) {
        const XrVector3f& xr_position = hand.joints()[joint_idx].pose.position;
        const XrQuaternionf& xr_rotation = hand.joints()[joint_idx].pose.orientation;
        const imp::float3 position{xr_position.x, xr_position.y, xr_position.z};
        const imp::quatf rotation{xr_rotation.w, xr_rotation.x, xr_rotation.y, xr_rotation.z};

        if(hand_mask_joints_[joint_idx])
        {
            if (joint_idx == XR_HAND_JOINT_WRIST_EXT)
            {   // Sets the world position of a given joint.
                hand_mask_joints_[joint_idx]->SetWorldPosition(position);
            }
            hand_mask_joints_[joint_idx]->SetWorldRotation(rotation);
            //outline_joints_[joint_idx]->SetWorldPosition(position);
        }
    }
    hand_mask_gltf_->ScheduleSkinningUpdate();
}

} // namespace ix::samsung::homecomponents
