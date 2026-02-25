#pragma once

#include <array>
#include "imp.h"
#include "proto/components/hand_renderer_state.proto.imp.h"
#if IMP_PLATFORM(ANDROID)
#include "native/openxr/hand_tracking/handtracking_actions.h"
#endif
namespace ix::samsung::homecomponents {

// Draws hand from a rigged 3D hand mesh.
class HandRenderer : public imp::Component {
public:
    enum class Handedness { LEFT, RIGHT };
    static constexpr int kNumJoints = 26;

    void Setup(Handedness handedness);
    void UpdateHandJoints(const HandActions& hand);

private:
    Handedness handType_;

    // Array storing the "hand mask" and "outline" bone nodes for each joint. "Hand mask" represnts
    // the parts of the hand that aren't rendered (creates the mask), and "outline" represents the
    // borders of the hand that actually get rendered.
    std::array<imp::NodeHandle, kNumJoints> hand_mask_joints_;
    //std::array<imp::NodeHandle, kNumJoints> outline_joints_;

    std::unique_ptr<imp::Material> hand_mask_material_;
    //std::unique_ptr<imp::Material> normal_outline_material_;

    imp::ComponentHandle<imp::GltfRenderer> hand_mask_gltf_;
    //imp::ComponentHandle<imp::GltfRenderer> outline_gltf_;

};

} // namespace ix::samsung::homecomponents
