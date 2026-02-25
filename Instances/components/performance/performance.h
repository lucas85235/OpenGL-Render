#ifndef COMPONENTS_PERFORMANCE_H
#define COMPONENTS_PERFORMANCE_H

#include "imp.h"
#include "core/view/base_view.h"
#include "core/monitor/monitor_helpers.h"
#include "core/monitor/monitor.h"
#include "core/monitor/duration_measurement_data.h"
#include "absl/strings/string_view.h"
#include "core/ncsb/component.h"
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace imp;
namespace ix::samsung::homecomponents {

    class PerformanceManager : public Component
    {
    public:
        void Setup(Monitor* monitor);

        void ResetDurationMeasurement();
        void IncreaseDurationMeasurement();
        void DisplayDurationMeasurement(const std::string& filename);

    private:
        struct TimeInfo {
            double view_frame_time;
            double filament_render_time;
            double view_advance_time;
        };

        imp::Monitor* monitor_ {};
        int count_ = 0;
        double fps_ = 0;
        double total_ms_ = 0;
        double min_ms_ = 0;
        double max_ms_ = 0;
        TimeInfo time_info_ {};

        absl::Duration GetLatestDurationMeasurement(absl::string_view measurement_id);
    };

} // namespace ix::samsung::homecomponents

#endif // COMPONENTS_PERFORMANCE_H