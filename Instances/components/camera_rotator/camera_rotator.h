#ifndef COMPONENTS_CAMERA_ROTATOR_H
#define COMPONENTS_CAMERA_ROTATOR_H

#include "imp.h"

namespace ix::samsung::homecomponents 
{

    using PointerId = imp::Pointer::Id;

    namespace Pointer {
        constexpr PointerId UNKNOWN = 0;
        constexpr PointerId LEFT = 1;
        constexpr PointerId RIGHT = 2;
        constexpr PointerId MOUSE = 3;

        constexpr PointerId PointerIds[3] =
                {LEFT,          RIGHT,          MOUSE};
    } // namespace Pointer

class CameraRotator : public imp::Component {
public:
    void Setup(float radians_per_pixel);

    void Update(const imp::FrameTime &frame_time);

private:
    // Set velocity limit very high (10k UI pixels per second).
    // Set acceleration limit such that we reach that limit quickly (30k UI pixels
    // per second^2 means we can reach max speed in about 1/3rd of a second.)
    // Set deceleration limit higher than acceleration limit, for snappiness.
    static constexpr auto kDragParameters = imp::SmoothParameters{10'000, 30'000, 50'000};

    float radians_per_pixel_;
    imp::float2 start_delta_;
    imp::Smooth <imp::float2> delta_;
    bool is_dragging_;
    bool at_target_ = false;
    bool enabled_ = false;
};

}  // namespace ix::samsung::homecomponents
#endif // COMPONENTS_CAMERA_ROTATOR_H
