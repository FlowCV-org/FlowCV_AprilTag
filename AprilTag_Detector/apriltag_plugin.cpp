//
// Plugin NDI Sender
//

#include "apriltag_plugin.hpp"
#include "imgui.h"
#include "imgui_instance_helper.hpp"

using namespace DSPatch;
using namespace DSPatchables;

int32_t global_inst_counter = 0;

namespace DSPatch::DSPatchables::internal
{
class AprilTagPlugin
{
};
}  // namespace DSPatch

AprilTagPlugin::AprilTagPlugin()
    : Component( ProcessOrder::OutOfOrder )
    , p( new internal::AprilTagPlugin() )
{
    // Name and Category
    SetComponentName_("AprilTag_Detector");
    SetComponentCategory_(Category::Category_Feature_Detection);
    SetComponentAuthor_("Richard");
    SetComponentVersion_("0.1.0");
    SetInstanceCount(global_inst_counter);
    global_inst_counter++;

    // 1 inputs
    SetInputCount_( 1, {"in"}, {IoType::Io_Type_CvMat} );

    // 2 outputs
    SetOutputCount_( 2, {"vis", "tags"}, {IoType::Io_Type_CvMat, IoType::Io_Type_JSON} );

    // Create Tag Family List
    tag_family_list_.emplace_back("Tag16h5");
    tag_family_list_.emplace_back("Tag25h7");
    tag_family_list_.emplace_back("Tag25h9");
    tag_family_list_.emplace_back("Tag36h9");
    tag_family_list_.emplace_back("Tag36h10");
    tag_family_list_.emplace_back("Tag36h11");

    should_init_ = true;
    tag_family_idx_ = 0;
    border_size_ = 2;
    low_tag_filter_ = 0;
    high_tag_filter_ = 50;
    filter_tags_ = false;
    init_success_ = false;
    is_initializing_ = false;

    SetEnabled(true);

}

void AprilTagPlugin::InitDetector_()
{
    if (io_mutex_.try_lock()) {
        while(proc_count_ > 0) {
        }
        tag_family_ = std::make_unique<TagFamily>(tag_family_list_.at(tag_family_idx_));
        tag_family_->blackBorder = border_size_;
        april_tag_detector_ = std::make_unique<TagDetector>(*tag_family_, params_);

        should_init_ = false;
        init_success_ = true;
        is_initializing_ = false;
        io_mutex_.unlock();
    }
}

void AprilTagPlugin::Process_( SignalBus const& inputs, SignalBus& outputs )
{
    // Handle Input Code Here
    auto in1 = inputs.GetValue<cv::Mat>(0);
    if (!in1) {
        return;
    }

    if (should_init_) {
        is_initializing_ = true;
        init_success_ = false;
        InitDetector_();
    }

    if (init_success_ && !is_initializing_) {
        proc_count_++;
        cv::Mat frame;
        if (!in1->empty()) {
            if (IsEnabled()) {
                in1->copyTo(frame);

                optical_center_.x = (float) frame.cols / 2.0f;
                optical_center_.y = (float) frame.rows / 2.0f;
                april_tag_detector_->process(frame, optical_center_, detections_);

                nlohmann::json json_out;
                nlohmann::json jTags;
                json_out["data_type"] = "apriltag";
                nlohmann::json ref;
                ref["w"] = frame.cols;
                ref["h"] = frame.rows;
                json_out["ref_frame"] = ref;

                if (!detections_.empty()) {
                    for (const auto &tag: detections_) {
                        if (filter_tags_) {
                            if (tag.id < low_tag_filter_ || tag.id > high_tag_filter_)
                                continue;
                        }
                        // Visualization
                        cv::circle(frame, tag.cxy, 2.0, cv::Scalar(0, 255, 0), 2.0);
                        cv::line(frame, tag.p[0], tag.p[1], cv::Scalar(255, 0, 0), 2);
                        cv::line(frame, tag.p[1], tag.p[2], cv::Scalar(0, 255, 0), 2);
                        cv::line(frame, tag.p[2], tag.p[3], cv::Scalar(0, 0, 255), 2);
                        cv::line(frame, tag.p[3], tag.p[0], cv::Scalar(255, 255, 0), 2);
                        std::string idStr = std::to_string((int) tag.id);
                        cv::Point txtCenter = tag.cxy;
                        txtCenter.y += 10;
                        cv::putText(frame, idStr, tag.cxy, cv::FONT_HERSHEY_COMPLEX_SMALL, 1.0, cv::Scalar(0, 0, 255), 2.0);

                        // JSON Data
                        nlohmann::json cTmp;
                        cTmp["id"] = tag.id;
                        nlohmann::json tCenter;
                        tCenter["x"] = tag.cxy.x;
                        tCenter["y"] = tag.cxy.y;
                        cTmp["center"] = tCenter;
                        nlohmann::json tCorners;
                        for (int i = 0; i < 4; i++) {
                            nlohmann::json pnt;
                            pnt["x"] = tag.p[i].x;
                            pnt["y"] = tag.p[i].y;
                            tCorners.emplace_back(pnt);
                        }
                        cTmp["corners"] = tCorners;
                        cTmp["rotation"] = tag.rotation;
                        jTags.emplace_back(cTmp);
                    }
                    json_out["data"] = jTags;
                }
                outputs.SetValue(1, json_out);
            } else {
                // Copy Original to Output (pass thru)
                in1->copyTo(frame);
            }

            if (!frame.empty())
                outputs.SetValue(0, frame);

        }
        proc_count_--;
    }
}

bool AprilTagPlugin::HasGui(int interface)
{
    // This is where you tell the system if your node has any of the following interfaces: Main, Control or Other
    if (interface == (int)FlowCV::GuiInterfaceType_Controls) {
        return true;
    }

    return false;
}

void AprilTagPlugin::UpdateGui(void *context, int interface)
{
    auto *imCurContext = (ImGuiContext *)context;
    ImGui::SetCurrentContext(imCurContext);

    if (interface == (int)FlowCV::GuiInterfaceType_Controls) {
        ImGui::SetNextItemWidth(120);
        if (ImGui::Combo(CreateControlString("Tag Family", GetInstanceName()).c_str(), &tag_family_idx_, [](void* data, int idx, const char** out_text) {
            *out_text = ((const std::vector<std::string>*)data)->at(idx).c_str();
            return true;
        }, (void*)&tag_family_list_, (int)tag_family_list_.size())) {
            should_init_ = true;
        }
        ImGui::Separator();
        ImGui::SetNextItemWidth(80);
        if (ImGui::DragInt(CreateControlString("Border Size", GetInstanceName()).c_str(), &border_size_, 0.25f, 1, 10)) {
            should_init_ = true;
        }
        ImGui::Separator();
        ImGui::Checkbox(CreateControlString("Filter Tags", GetInstanceName()).c_str(), &filter_tags_);
        if (filter_tags_) {
            ImGui::SetNextItemWidth(100);
            ImGui::DragInt(CreateControlString("Min Tag ID", GetInstanceName()).c_str(), &low_tag_filter_, 0.25f, 0, 1000);
            ImGui::SetNextItemWidth(100);
            ImGui::DragInt(CreateControlString("Max Tag ID", GetInstanceName()).c_str(), &high_tag_filter_, 0.25f, 0, 1000);
        }
    }
}

std::string AprilTagPlugin::GetState()
{
    using namespace nlohmann;

    json state;

    state["family"] = tag_family_list_.at(tag_family_idx_);
    state["family_idx"] = tag_family_idx_;
    state["border_size"] = border_size_;
    state["filter_tags"] = filter_tags_;
    if (filter_tags_) {
        state["low_filter_id"] = low_tag_filter_;
        state["high_filter_id"] = high_tag_filter_;
    }

    std::string stateSerialized = state.dump(4);

    return stateSerialized;
}

void AprilTagPlugin::SetState(std::string &&json_serialized)
{
    using namespace nlohmann;

    json state = json::parse(json_serialized);

    if (state.contains("family_idx")) {
        tag_family_idx_ = state["family_idx"].get<int>();
        if (tag_family_idx_ > tag_family_list_.size())
            tag_family_idx_ = 0;
    }
    if (state.contains("family")) {
        auto tagFamTmp = state["family"].get<std::string>();
        if (tagFamTmp != tag_family_list_.at(tag_family_idx_)) {
            bool found = false;
            for (int i = 0; i < tag_family_list_.size(); i++) {
                if (tag_family_list_.at(i) == tagFamTmp) {
                    tag_family_idx_ = i;
                    found = true;
                    break;
                }
            }
            if (!found)
                tag_family_idx_ = 0;
        }
    }
    if (state.contains("border_size"))
        border_size_ = state["border_size"].get<int>();
    if (state.contains("filter_tags"))
        filter_tags_ = state["filter_tags"].get<bool>();
    if (state.contains("low_filter_id"))
        low_tag_filter_ = state["low_filter_id"].get<int>();
    if (state.contains("high_filter_id"))
        high_tag_filter_ = state["high_filter_id"].get<int>();


}
