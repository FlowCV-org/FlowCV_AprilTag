//
// Plugin AprilTag Detector
//

#ifndef FLOWCV_APRILTAG_DETECTOR_PLUGIN_HPP_
#define FLOWCV_APRILTAG_DETECTOR_PLUGIN_HPP_
#include <DSPatch.h>
#include "FlowCV_Types.hpp"
#include "json.hpp"
#include "opencv2/opencv.hpp"
#include <CameraUtil.h>
#include <TagDetector.h>
#include <iostream>
#include <vector>
#include <atomic>
#include <string>

namespace DSPatch::DSPatchables
{
namespace internal
{
class AprilTagPlugin;
}

class DLLEXPORT AprilTagPlugin final : public Component
{
  public:
    AprilTagPlugin();
    void UpdateGui(void *context, int interface) override;
    bool HasGui(int interface) override;
    std::string GetState() override;
    void SetState(std::string &&json_serialized) override;

  protected:
    void Process_( SignalBus const& inputs, SignalBus& outputs ) override;
    void InitDetector_();

  private:
    std::unique_ptr<internal::AprilTagPlugin> p;
    std::mutex io_mutex_;
    std::unique_ptr<TagDetector> april_tag_detector_;
    std::unique_ptr<TagFamily> tag_family_;
    std::vector<std::string> tag_family_list_;
    TagDetectorParams params_;
    int border_size_;
    TagDetectionArray detections_;
    cv::Point2d optical_center_;
    int tag_family_idx_;
    int low_tag_filter_;
    int high_tag_filter_;
    bool filter_tags_;
    bool init_success_;
    bool should_init_;
    std::atomic_bool is_initializing_{};
    std::atomic_int proc_count_{};

};

EXPORT_PLUGIN( AprilTagPlugin )

}  // namespace DSPatch::DSPatchables

#endif //FLOWCV_APRILTAG_DETECTOR_PLUGIN_HPP_
