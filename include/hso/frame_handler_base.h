// This file is part of HSO: Hybrid Sparse Monocular Visual Odometry 
// With Online Photometric Calibration
//
// Copyright(c) 2021, Dongting Luo, Dalian University of Technology, Dalian
// Copyright(c) 2021, Robotics Group, Dalian University of Technology
//
// This program is highly based on the previous implementation 
// of SVO: https://github.com/uzh-rpg/rpg_svo
// and PL-SVO: https://github.com/rubengooj/pl-svo
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#ifndef HSO_FRAME_HANDLER_BASE_H_
#define HSO_FRAME_HANDLER_BASE_H_

#include <queue>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/thread.hpp>
#include <hso/global.h>
#include <hso/map.h>

#include "hso/vikit/timer.h"
#include "hso/vikit/ringbuffer.h"

namespace hso {
class PerformanceMonitor;}

namespace hso
{
class Point;
class Matcher;
class DepthFilter;

/// Base class for various VO pipelines. Manages the map and the state machine.
class FrameHandlerBase : boost::noncopyable
{
public:
  enum Stage {
    STAGE_PAUSED,
    STAGE_FIRST_FRAME, //处理第一帧
    STAGE_SECOND_FRAME, // 处理第二帧
    STAGE_DEFAULT_FRAME, // 常规跟踪，即初始化成功后系统进入常规跟踪
    STAGE_RELOCALIZING // 重定位
  };
  enum TrackingQuality {
    TRACKING_INSUFFICIENT, // 跟踪到的特征太少(匹配点少，不可信)
    TRACKING_BAD, // 跟踪质量不好，丢失特征太多
    TRACKING_GOOD // 跟踪质量好
  };
  enum UpdateResult {
    RESULT_NO_KEYFRAME, // 当前帧是关键帧
    RESULT_IS_KEYFRAME, // 当前帧不是关键帧
    RESULT_FAILURE // 当前帧跟踪失败
  };

  FrameHandlerBase();

  virtual ~FrameHandlerBase();

  /// Get the current map.
  const Map& map() const { return map_; }

  /// Will reset the map as soon as the current frame is finished processing.
  void reset() { set_reset_ = true; }

  /// Start processing.
  void start() { set_start_ = true; }

  /// Get the current stage of the algorithm.
  Stage stage() const { return stage_; }

  /// Get tracking quality.
  TrackingQuality trackingQuality() const { return tracking_quality_; }

  /// Get the processing time of the previous iteration.
  double lastProcessingTime() const { return timer_.getTime(); }

  /// Get the number of feature observations of the last frame.
  size_t lastNumObservations() const { return num_obs_last_; }

public:
  Stage stage_;                 //!< Current stage of the algorithm.
  bool set_reset_;              //!< Flag that the user can set. Will reset the system before the next iteration.
  bool set_start_;              //!< Flag the user can set to start the system when the next image is received.
  Map map_;                     //!< Map of keyframes created by the slam system.
  hso::Timer timer_;            //!< Stopwatch to measure time to process frame.
  hso::RingBuffer<double> acc_frame_timings_;    //!< Total processing time of the last 10 frames, used to give some user feedback on the performance.
  hso::RingBuffer<size_t> acc_num_obs_;          //!< Number of observed features of the last 10 frames, used to give some user feedback on the tracking performance.
  size_t num_obs_last_;                         //!< Number of observations in the previous frame.
  TrackingQuality tracking_quality_;            //!< An estimate of the tracking quality based on the number of tracked features.
  size_t regular_counter_; // 表示距离上一关键帧经过的普通帧帧数

  /// Before a frame is processed, this function is called.
  bool startFrameProcessingCommon(const double timestamp);

  /// When a frame is finished processing, this function is called.
  int finishFrameProcessingCommon(
      const size_t update_id,
      const UpdateResult dropout,
      const size_t num_observations);

  /// Reset the map and frame handler to start from scratch.
  void resetCommon();

  /// Reset the frame handler. Implement in derived class.
  virtual void resetAll() { resetCommon(); }

  /// Set the tracking quality based on the number of tracked features.
  virtual void setTrackingQuality(const size_t num_observations);

  /// Optimize some of the observed 3D points.
  virtual void optimizeStructure(FramePtr frame, size_t max_n_pts, int max_iter);
};

} // namespace nslam

#endif // HSO_FRAME_HANDLER_BASE_H_
