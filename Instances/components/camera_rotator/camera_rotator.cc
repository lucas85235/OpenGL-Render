#include "camera_rotator.h"

using namespace imp;

namespace ix::samsung::homecomponents
{
    void CameraRotator::Setup(float radians_per_pixel)
    {
        radians_per_pixel_ = radians_per_pixel;

        Dispatcher& dispatcher = GetView().GetDispatcher();
        dispatcher.Connect(
            [this](const DragGesture::StartEvent& event) mutable
            {
                if(!enabled_) return;

                is_dragging_ = true;
                delta_.Setup(kDragParameters, start_delta_);
                delta_.SetTarget(start_delta_ + event.activation_delta);
                at_target_ = false;
            },
            this);
        dispatcher.Connect(
            [this](const DragGesture::UpdateEvent& event) mutable
            {
                if(!enabled_) return;

                delta_.SetTarget(delta_.GetTarget() + event.delta);
                at_target_ = false;

            },
            this);

        dispatcher.Connect(
            [this](const DragGesture::FinishEvent& event) mutable
            {
                if(!enabled_) return;

                is_dragging_ = false;
                start_delta_ = delta_.Get();
            },
            this);

        dispatcher.Connect(
            [this](const KeyboardEvent& event) mutable
            {
                switch (event.type)
                {
                case KeyboardEventType::kOnDown:
                    {
                        if(event.key.code == VirtualKeyCode::VK_v && !enabled_)
                        {
                            imp::output::Error("Pressed V key");
                            enabled_ = true;
                        }
                        break;
                    }

                case KeyboardEventType::kOnUp:
                    {
                        if(event.key.code == VirtualKeyCode::VK_v && enabled_)
                        {
                            imp::output::Error("Released V key");
                            enabled_ = false;
                        }
                        break;
                    }
                }
            },
            this);

        is_dragging_ = false;
    }

    void CameraRotator::Update(const FrameTime& frame_time)
    {
        if (!is_dragging_ || at_target_)
        {
            return;
        }
        // Update our smooth interpolator and remember if it stopped.
        at_target_ = delta_.Step(frame_time.GetDeltaSeconds());

        // Apply accumulated delta to the local rotation.
        float2 delta_rotation = delta_.Get() * radians_per_pixel_;
        quatf xRotation = quatf::fromAxisAngle(kUp, delta_rotation.x);
        quatf yRotation = quatf::fromAxisAngle(kRight, delta_rotation.y);

        GetNode()->SetLocalRotation(xRotation * yRotation);
    }
} // ix::samsung::homecomponents
