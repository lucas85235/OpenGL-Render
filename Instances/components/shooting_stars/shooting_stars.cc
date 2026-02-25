#include "native/components/shooting_stars/shooting_stars.h"

#include "imp.h"

#include <cmath>
#include <tuple>
#include <utility>
#include <cstdlib>
#include <ctime>

namespace ix::moohan::home_support {

    // Set initial animation time and self update with proto
    float kCurrentAnimationTime = 3.0f;

    // Time between stars
    float kAnimationSleepTimeSec = 1.2f;

    // Panel size with a quad proportion
    float kPanelSize = 4.0f;

    // Shooting star size width e lenght
    imp::float2 kShootingStarAngles = { 150.0, 30.0 };
    bool kIsFixedOrRandomAngles = false; // false = fixe | true = random

    // Shooting star size width e lenght
    imp::float2 kShootingStarSize = { 0.03f, 0.5f };

    // Shooting star fixed color
    imp::float4 kShootingStarColor = {1.0f, 0.996f, 0.973f, 1.0f};

    // Random spaw position area
    constexpr imp::float2 kDefaultRandomXMinMax = { -30.0f, 30.0f };
    constexpr imp::float2 kDefaultRandomYMinMax = { 27.0f, 30.0f };
    constexpr imp::float2 kDefaultRandomZMinMax = { -10.0, -10.0f };
    constexpr float kDefaultPanelAngleX = 30.0f;

    void ShootingStars::Setup(imp::resources::ResourceDefinition asset) {
        renderer_ = GetNode()->GetComponent<imp::RenderComponent>();
        GetNode()->SetLocalScale({1.0, 1.0, 1.0});
        owner_ = GetNode();
        camera_node_ = GetView().GetCameraManager().GetCamera()->GetNode();
        auto shooting_stars_material_future = GetView().GetAssetManager().LoadMaterial(asset);

        shooting_stars_material_future.Then([this](imp::AssetPtr<imp::MaterialAsset> material_asset) mutable {
            shooting_stars_material_asset_ = material_asset;
            imp::MaterialPtr material = GetView().GetMaterialFactory().CreateMaterial(shooting_stars_material_asset_);

            material->SetName("Shooting_Stars");
            shooting_stars_material_ptr_ = material.get();
            renderer_->SetMaterial(std::move(material));

            // Setup initial parameters values, only in Impress-Object-Manipulation
            state_.animation_time_sec = kCurrentAnimationTime;
            state_.animation_sleep_time_sec = kAnimationSleepTimeSec;
            state_.panel_size = kPanelSize;
            state_.shooting_star_angles = kShootingStarAngles;
            state_.is_fixed_or_random_angles = kIsFixedOrRandomAngles;
            state_.shooting_star_size = kShootingStarSize;
            state_.base_color = kShootingStarColor;
            state_.x_panel_angle = kDefaultPanelAngleX;

            OnIsfStateChanged();
            ResetAnimation();

            imp::output::Info("[Shooting_Stars] Set Shooting_Stars custom Material!");
        }).KeptBy(this);
    }

    void ShootingStars::Update(const imp::FrameTime& frame_time) {
        if (count_time_ >= kCurrentAnimationTime && can_animate == true) {
            StopAnimation();
        }
        else if (count_time_ >= (kCurrentAnimationTime + kAnimationSleepTimeSec)) {
            ResetAnimation();
        }
        else {
            InProgressAnimation(frame_time.GetDeltaSeconds());
        }
    }

    void ShootingStars::LookAtCamera()
    {
        auto transform = Transform<float>(mat4f::lookAt(camera_node_->GetWorldPosition(), owner_->GetWorldPosition(), samsung::homecomponents::MathUtils::GetUpVector(owner_)));
        owner_->SetWorldRotation(transform.rotation);
    }

    void ShootingStars::InProgressAnimation(float delta_seconds)
    {
        shooting_stars_material_ptr_->SetParameter("deltaTime", count_time_);
        count_time_ += delta_seconds;
    }

    void ShootingStars::StopAnimation()
    {
        can_animate = false;
        renderer_->SetEnabled(false);
    }

    void ShootingStars::ResetAnimation()
    {
        OnIsfStateChanged();
        ApplyDelayedRandomPosition();

        count_time_ = 0;
        shooting_stars_material_ptr_->SetParameter("deltaTime", count_time_);
        renderer_->SetEnabled(true);
        can_animate = true;
        LookAtCamera();
    }

    void ShootingStars::ApplyDelayedRandomPosition() {
        GetNode()->SetLocalPosition({
            GetRandom(random_float_x_.x, random_float_x_.y),
            GetRandom(random_float_y_.x, random_float_y_.y),
            GetRandom(random_float_z_.x, random_float_z_.y)
        });

        float random = Random01();
        shooting_stars_material_ptr_->SetParameter("random", random);

        random = Random01();
        shooting_stars_material_ptr_->SetParameter("randomSizeX", random);
    }

    void ShootingStars::OnIsfStateChanged() {
        if (shooting_stars_material_ptr_ == nullptr) {
            imp::output::Info("Null Reference!");
            return;
        }

        kCurrentAnimationTime = state_.animation_time_sec.value_or(kCurrentAnimationTime);
        kAnimationSleepTimeSec = state_.animation_sleep_time_sec.value_or(kAnimationSleepTimeSec);
        ChangePanelSize(state_.panel_size.value_or(kPanelSize));
        kShootingStarSize = state_.shooting_star_size.value_or(kShootingStarSize);
        kShootingStarAngles = state_.shooting_star_angles.value_or(kShootingStarAngles);
        kIsFixedOrRandomAngles = state_.is_fixed_or_random_angles.value_or(kIsFixedOrRandomAngles);
        kShootingStarColor = state_.base_color.value_or(kShootingStarColor);

        if (kIsFixedOrRandomAngles == false) {
            bool condition = Random01() > 0.5;
            float angle = condition ? kShootingStarAngles.x : kShootingStarAngles.y;
            shooting_stars_material_ptr_->SetParameter("starAngle", angle);
        }
        else {
            float random_angle = RandomValues(kShootingStarAngles.x, kShootingStarAngles.y);
            shooting_stars_material_ptr_->SetParameter("starAngle", random_angle);
        }

        shooting_stars_material_ptr_->SetParameter("animationTime", kCurrentAnimationTime);
        shooting_stars_material_ptr_->SetParameter("starSize", kShootingStarSize);
        shooting_stars_material_ptr_->SetParameter("baseColor", kShootingStarColor);

        imp::float2 x_min_max = state_.x_position_min_max.value_or(kDefaultRandomXMinMax);
        imp::float2 y_min_max = state_.y_position_min_max.value_or(kDefaultRandomYMinMax);
        imp::float2 z_min_max = state_.z_position_min_max.value_or(kDefaultRandomZMinMax);

        random_float_x_.x = x_min_max.x;
        random_float_x_.y = x_min_max.y;
        random_float_y_.x = y_min_max.x;
        random_float_y_.y = y_min_max.y;
        random_float_z_.x = z_min_max.x;
        random_float_z_.y = z_min_max.y;

        float x_panel_angle = state_.x_panel_angle.value_or(kDefaultPanelAngleX);
        imp::quatf delta_rotation = imp::quatf::fromAxisAngle(imp::kRight, imp::ToRadians(x_panel_angle));
        GetNode()->SetWorldRotation(delta_rotation);

        imp::output::Info("OnIsfStateChanged");
    }

    void ShootingStars::ChangePanelSize(float panel_size)
    {
        GetNode()->SetLocalScale({panel_size, panel_size, 1.0f});
        shooting_stars_material_ptr_->SetParameter("panelSize", imp::float2{panel_size, panel_size});
    }

    float ShootingStars::Lerp(float a, float b, float f)
    {
        return a * (1.0 - f) + (b * f);
    }

    float ShootingStars::Random01()
    {
        return rand() % 100 / 100.0;
    }

    float ShootingStars::RandomValues(float a, float b)
    {
        return Lerp(a, b, Random01());
    }

    float ShootingStars::GetRandom(float min, float max)
    {
        if (!initialized) {
            std::srand(static_cast<unsigned int>(std::time(0)));
            initialized = true;
        }

        float random = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        return min + random * (max - min);
    }

}  // ix::moohan::home_support
