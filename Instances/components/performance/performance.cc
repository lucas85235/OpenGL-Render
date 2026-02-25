#include "./performance.h"
#include "imp.h"

using namespace imp;
namespace fs = std::filesystem;

namespace ix::samsung::homecomponents {

    void PerformanceManager::Setup(imp::Monitor *monitor) {
        monitor_ = monitor;
    }

    absl::Duration PerformanceManager::GetLatestDurationMeasurement(absl::string_view measurement_id) {
        imp::MeasurementData::MeasurementId id = monitor_->GetMeasurementId(measurement_id);
        imp::DurationMeasurementData* data =
                static_cast<imp::DurationMeasurementData*>(monitor_->GetMeasurementData(id));
        return data->GetLatestSampleDuration();
    }

    void PerformanceManager::ResetDurationMeasurement()
    {
        count_ = 0;
        time_info_ = {};
        total_ms_ = 0;
        fps_ = 0;
        min_ms_ = 0;
        max_ms_ = 0;
    }

    void PerformanceManager::IncreaseDurationMeasurement()
    {
        count_++;

        auto viewFrameTime = absl::ToDoubleMilliseconds(
                GetLatestDurationMeasurement(imp::kViewFrameTime));

        if (count_ == 1 || viewFrameTime < min_ms_)
            min_ms_ = viewFrameTime;

        if (viewFrameTime > max_ms_)
            max_ms_ = viewFrameTime;

        time_info_.view_frame_time += viewFrameTime;
        time_info_.filament_render_time += absl::ToDoubleMilliseconds(
                GetLatestDurationMeasurement(imp::kFilamentFrameTiming));
        time_info_.view_advance_time += absl::ToDoubleMilliseconds(
                GetLatestDurationMeasurement(imp::kViewAdvance));
    }

    void PerformanceManager::DisplayDurationMeasurement(const std::string& filename)
    {
        total_ms_ = time_info_.view_frame_time / count_;
        fps_ = 1000 / total_ms_;
        auto min_fps = 1000 / max_ms_;
        auto max_fps = 1000 / min_ms_;

        // Diretory to the output file
        fs::path new_dir = "performance-output";

        if (!fs::is_directory(new_dir)){
            fs::create_directory(new_dir.c_str());
            output::Info("Directory created.");
        }

        // Opening the file for writing
        std::ofstream file(filename);
        if (!file.is_open()) {
            output::Error("Error to open the file %s to writing.", filename);
            return;
        }

        // Writing the JSON header
        file << "{\n";

        // Writing the key:value pairs
        file << "  \"" << "Fps average" << "\": \"" << round(fps_) << "\"";
        file << "," << "\n";
        file << "  \"" << "Fps min" << "\": \"" << round(min_fps) << "\"";
        file << "," << "\n";
        file << "  \"" << "Fps max" << "\": \"" << round(max_fps) << "\"";
        file << "," << "\n";
        file << "  \"" << "Filament Render Time" << "\": \"" <<  time_info_.filament_render_time / count_ << "\"";
        file << "," << "\n";
        file << "  \"" << "View Advance time" << "\": \"" << time_info_.view_advance_time / count_ << "\"";
        file << "\n";

        // Writing JSON closure
        file << "}\n";

        output::Info("JSON file created success: %s", filename);
    }

} // namespace ix::samsung::homecomponents
