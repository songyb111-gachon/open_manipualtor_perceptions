﻿#ifndef OPEN_MANIPULATOR_PICK_AND_PLACE_H
#define OPEN_MANIPULATOR_PICK_AND_PLACE_H

#include <ros/ros.h>
#include <termios.h>
#include <sys/ioctl.h>

#include "open_manipulator_msgs/OpenManipulatorState.h"
#include "open_manipulator_msgs/KinematicsPose.h"
#include "open_manipulator_msgs/SetJointPosition.h"
#include "open_manipulator_msgs/SetKinematicsPose.h"

#include "ar_track_alvar_msgs/AlvarMarkers.h"
#include "sensor_msgs/JointState.h"

#define NUM_OF_JOINT_AND_TOOL 5
#define HOME_POSE   1
#define DEMO_START  2
#define DEMO_STOP   3

typedef struct _ArMarker
{
  uint32_t id;
  double position[3];
} ArMarker;

class OpenManipulatorPickandPlace
{
 private:
  // ROS NodeHandle
  ros::NodeHandle node_handle_;
  ros::NodeHandle priv_node_handle_;
  ros::ServiceClient goal_joint_space_path_client_;
  ros::ServiceClient goal_tool_control_client_;
  ros::ServiceClient goal_task_space_path_client_;

  ros::Subscriber open_manipulator_states_sub_;
  ros::Subscriber open_manipulator_joint_states_sub_;
  ros::Subscriber open_manipulator_kinematics_pose_sub_;
  ros::Subscriber ar_pose_marker_sub_;

  std::vector<double> present_joint_angle_;            // 현재 조인트 각도
  std::vector<double> present_kinematic_position_;     // 현재 위치
  std::vector<double> present_kinematic_orientation_;  // 현재 오리엔테이션
  std::vector<std::string> joint_name_;
  bool open_manipulator_is_moving_;
  std::vector<ArMarker> ar_marker_pose;

  uint8_t mode_state_;
  uint8_t demo_count_;
  uint8_t pick_ar_id_;

 public:
  OpenManipulatorPickandPlace();
  ~OpenManipulatorPickandPlace();

  void initServiceClient();
  void initSubscribe();

  void manipulatorStatesCallback(const open_manipulator_msgs::OpenManipulatorState::ConstPtr &msg);
  void kinematicsPoseCallback(const open_manipulator_msgs::KinematicsPose::ConstPtr &msg);
  void jointStatesCallback(const sensor_msgs::JointState::ConstPtr &msg);
  void arPoseMarkerCallback(const ar_track_alvar_msgs::AlvarMarkers::ConstPtr &msg);

  bool setJointSpacePath(std::vector<std::string> joint_name, std::vector<double> joint_angle, double path_time);
  bool setToolControl(std::vector<double> joint_angle);
  bool setTaskSpacePath(std::vector<double> kinematics_pose, std::vector<double> kienmatics_orientation, double path_time);

  void publishCallback(const ros::TimerEvent&);
  void setModeState(char ch);
  void demoSequence();

  void printText();
  bool kbhit();
};

#endif //OPEN_MANIPULATOR_PICK_AND_PLACE_H
