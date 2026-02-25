#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "imp.h"

#include "native/components/hand/hand_renderer.h"
#include "proto/components/hand_state.proto.imp.h"

namespace ix::samsung::homecomponents {

// Impress component to draw hands.
class Hand : public imp::Component {
public:
    void Setup(HandRenderer::Handedness handType);
    void Update(const imp::FrameTime& deltaTime);
    void UpdateHandActions();

private:
    HandRenderer::Handedness handType_;
    imp::ComponentHandle<HandRenderer> hand_renderer_;

public:
    //using IsfInfo = imp::IsfInfo<&Hand::state_>;
};

} // ix::samsung::homecomponents
