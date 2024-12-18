﻿/*******************************************************************************
* Copyright 2018 ROBOTIS CO., LTD.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

/* Authors: Darby Lim, Hye-Jong KIM, Ryan Shim, Yong-Ho Na */

#include "open_manipulator_pick_and_place/open_manipulator_pick_and_place.h"

OpenManipulatorPickandPlace::OpenManipulatorPickandPlace()
: node_handle_(""),
  priv_node_handle_("~"),
  mode_state_(0),
  demo_count_(0),
  pick_ar_id_(0)
{
  present_joint_angle_.resize(NUM_OF_JOINT_AND_TOOL, 0.0);
  present_kinematic_position_.resize(3, 0.0);

  joint_name_.push_back("joint1");
  joint_name_.push_back("joint2");
  joint_name_.push_back("joint3");
  joint_name_.push_back("joint4");

  initServiceClient();
  initSubscribe();
}

OpenManipulatorPickandPlace::~OpenManipulatorPickandPlace()
{
  if (ros::isStarted())
  {
    ros::shutdown();
    ros::waitForShutdown();
  }
}

void OpenManipulatorPickandPlace::initServiceClient()
{
  goal_joint_space_path_client_ = node_handle_.serviceClient<open_manipulator_msgs::SetJointPosition>("goal_joint_space_path");
  goal_tool_control_client_ = node_handle_.serviceClient<open_manipulator_msgs::SetJointPosition>("goal_tool_control");
  goal_task_space_path_client_ = node_handle_.serviceClient<open_manipulator_msgs::SetKinematicsPose>("goal_task_space_path");
}

void OpenManipulatorPickandPlace::initSubscribe()
{
  open_manipulator_states_sub_ = node_handle_.subscribe("states", 10, &OpenManipulatorPickandPlace::manipulatorStatesCallback, this);
  open_manipulator_joint_states_sub_ = node_handle_.subscribe("joint_states", 10, &OpenManipulatorPickandPlace::jointStatesCallback, this);
  open_manipulator_kinematics_pose_sub_ = node_handle_.subscribe("gripper/kinematics_pose", 10, &OpenManipulatorPickandPlace::kinematicsPoseCallback, this);
  ar_pose_marker_sub_ = node_handle_.subscribe("/ar_pose_marker", 10, &OpenManipulatorPickandPlace::arPoseMarkerCallback, this);
}

bool OpenManipulatorPickandPlace::setJointSpacePath(std::vector<std::string> joint_name, std::vector<double> joint_angle, double path_time)
{
  open_manipulator_msgs::SetJointPosition srv;
  srv.request.joint_position.joint_name = joint_name;
  srv.request.joint_position.position = joint_angle;
  srv.request.path_time = path_time;

  if (goal_joint_space_path_client_.call(srv))
  {
    return srv.response.is_planned;
  }
  return false;
}

bool OpenManipulatorPickandPlace::setToolControl(std::vector<double> joint_angle)
{
  open_manipulator_msgs::SetJointPosition srv;
  srv.request.joint_position.joint_name.push_back("gripper");
  srv.request.joint_position.position = joint_angle;

  if (goal_tool_control_client_.call(srv))
  {
    return srv.response.is_planned;
  }
  return false;
}

bool OpenManipulatorPickandPlace::setTaskSpacePath(std::vector<double> kinematics_pose,std::vector<double> kienmatics_orientation, double path_time)
{
  open_manipulator_msgs::SetKinematicsPose srv;

  srv.request.end_effector_name = "gripper";

  srv.request.kinematics_pose.pose.position.x = kinematics_pose.at(0);
  srv.request.kinematics_pose.pose.position.y = kinematics_pose.at(1);
  srv.request.kinematics_pose.pose.position.z = kinematics_pose.at(2);

  srv.request.kinematics_pose.pose.orientation.w = kienmatics_orientation.at(0);
  srv.request.kinematics_pose.pose.orientation.x = kienmatics_orientation.at(1);
  srv.request.kinematics_pose.pose.orientation.y = kienmatics_orientation.at(2);
  srv.request.kinematics_pose.pose.orientation.z = kienmatics_orientation.at(3);

  srv.request.path_time = path_time;

  if (goal_task_space_path_client_.call(srv))
  {
    return srv.response.is_planned;
  }
  return false;
}

void OpenManipulatorPickandPlace::manipulatorStatesCallback(const open_manipulator_msgs::OpenManipulatorState::ConstPtr &msg)
{
  if (msg->open_manipulator_moving_state == msg->IS_MOVING)
    open_manipulator_is_moving_ = true;
  else
    open_manipulator_is_moving_ = false;
}

void OpenManipulatorPickandPlace::jointStatesCallback(const sensor_msgs::JointState::ConstPtr &msg)
{
  std::vector<double> temp_angle;
  temp_angle.resize(NUM_OF_JOINT_AND_TOOL);
  for (int i = 0; i < msg->name.size(); i ++)
  {
    if (!msg->name.at(i).compare("joint1"))  temp_angle.at(0) = (msg->position.at(i));
    else if (!msg->name.at(i).compare("joint2"))  temp_angle.at(1) = (msg->position.at(i));
    else if (!msg->name.at(i).compare("joint3"))  temp_angle.at(2) = (msg->position.at(i));
    else if (!msg->name.at(i).compare("joint4"))  temp_angle.at(3) = (msg->position.at(i));
    else if (!msg->name.at(i).compare("gripper"))  temp_angle.at(4) = (msg->position.at(i));
  }
  present_joint_angle_ = temp_angle;
}

void OpenManipulatorPickandPlace::kinematicsPoseCallback(const open_manipulator_msgs::KinematicsPose::ConstPtr &msg)
{
  std::vector<double> temp_position;
  temp_position.push_back(msg->pose.position.x);
  temp_position.push_back(msg->pose.position.y);
  temp_position.push_back(msg->pose.position.z);

  present_kinematic_position_ = temp_position;
}

void OpenManipulatorPickandPlace::arPoseMarkerCallback(const ar_track_alvar_msgs::AlvarMarkers::ConstPtr &msg)
{
  std::vector<ArMarker> temp_buffer;
  for (int i = 0; i < msg->markers.size(); i ++)
  {
    ArMarker temp;
    temp.id = msg->markers.at(i).id;
    temp.position[0] = msg->markers.at(i).pose.pose.position.x;
    temp.position[1] = msg->markers.at(i).pose.pose.position.y;
    temp.position[2] = msg->markers.at(i).pose.pose.position.z;

    temp_buffer.push_back(temp);
  }

  ar_marker_pose = temp_buffer;
}

void OpenManipulatorPickandPlace::publishCallback(const ros::TimerEvent&)
{
  printText();
  if (kbhit()) setModeState(std::getchar());

  if (mode_state_ == HOME_POSE)
  {
    std::vector<double> joint_angle;

    joint_angle.push_back( 0.01);
    joint_angle.push_back(-0.80);
    joint_angle.push_back( 0.00);
    joint_angle.push_back( 1.90);
    setJointSpacePath(joint_name_, joint_angle, 2.0);

    std::vector<double> gripper_value;
    gripper_value.push_back(0.0);
    setToolControl(gripper_value);
    mode_state_ = 0;
  }
  else if (mode_state_ == DEMO_START)
  {
    if (!open_manipulator_is_moving_) demoSequence();
  }
  else if (mode_state_ == DEMO_STOP)
  {

  }
}
void OpenManipulatorPickandPlace::setModeState(char ch)
{
  if (ch == '1')
    mode_state_ = HOME_POSE;
  else if (ch == '2')
  {
    mode_state_ = DEMO_START;
    demo_count_ = 0;
  }
  else if (ch == '3')
    mode_state_ = DEMO_STOP;
}

void OpenManipulatorPickandPlace::demoSequence()
{
  std::vector<double> joint_angle;
  std::vector<double> kinematics_position;
  std::vector<double> kinematics_orientation;
  std::vector<double> gripper_value;

  switch (demo_count_)
  {
    case 0: // home pose
      joint_angle.push_back( 0.00);
    joint_angle.push_back(-1.05);
    joint_angle.push_back( 0.35);
    joint_angle.push_back( 0.70);
    setJointSpacePath(joint_name_, joint_angle, 2.0);
    demo_count_ ++;
    break;
    case 1: // initial pose
      joint_angle.push_back( 0.01);
    joint_angle.push_back(-0.80);
    joint_angle.push_back( 0.00);
    joint_angle.push_back( 1.90);
    setJointSpacePath(joint_name_, joint_angle, 2.0);
    demo_count_ ++;
    break;
    case 2: // wait & open the gripper
      setJointSpacePath(joint_name_, present_joint_angle_, 3.0);
    gripper_value.push_back(0.010);
    setToolControl(gripper_value);
    demo_count_ ++;
    break;

    case 3: // pick the box for marker 0
    {
      bool marker_found = false;
      kinematics_position.clear();
      kinematics_orientation.clear();
      for (int i = 0; i < ar_marker_pose.size(); i++)
      {
        if (ar_marker_pose.at(i).id == 0) // ID 0 마커 확인
        {
          marker_found = true;
          // X, Y, Z 값을 설정
          kinematics_position.push_back(ar_marker_pose.at(i).position[0] + 0.005); // X 좌표
          kinematics_position.push_back(ar_marker_pose.at(i).position[1]);        // Y 좌표
          kinematics_position.push_back(0.033);                                  // Z 좌표 고정

          // 오리엔테이션 설정
          kinematics_orientation.push_back(0.74); // w 값
          kinematics_orientation.push_back(0.00); // x 값
          kinematics_orientation.push_back(0.66); // y 값
          kinematics_orientation.push_back(0.00); // z 값

          setTaskSpacePath(kinematics_position, kinematics_orientation, 3.0);
          demo_count_++; // 다음 단계로 진행
          break; // 찾았으므로 반복문 종료
        }
      }

      if (!marker_found)
      {
        printf("Marker 0 not detected.\n");
        demo_count_ = 1;
      }
    }
  break;

  case 4: // wait & grip
    setJointSpacePath(joint_name_, present_joint_angle_, 1.0);
    gripper_value.clear();
    gripper_value.push_back(-0.008);
    setToolControl(gripper_value);
    demo_count_++;
    break;

  case 5: // initial pose
    joint_angle.clear();
    joint_angle.push_back( 0.01);
    joint_angle.push_back(-0.80);
    joint_angle.push_back( 0.00);
    joint_angle.push_back( 1.90);
    setJointSpacePath(joint_name_, joint_angle, 2.0);
    demo_count_++;
    break;

  case 6: // place pose
    joint_angle.clear();
    joint_angle.push_back( 1.57);
    joint_angle.push_back(-0.21);
    joint_angle.push_back(-0.15);
    joint_angle.push_back( 1.89);
    setJointSpacePath(joint_name_, joint_angle, 2.0);
    demo_count_++;
    break;

    case 7: // place the box
    kinematics_position.clear();
    kinematics_orientation.clear();

    // 수정된 위치 값
    kinematics_position.push_back(0.015); // X 좌표
    kinematics_position.push_back(0.123); // Y 좌표
    kinematics_position.push_back(0.065); // Z 좌표

    // 기존 오리엔테이션 값 유지
    kinematics_orientation.push_back(0.74); // w 값
    kinematics_orientation.push_back(0.00); // x 값
    kinematics_orientation.push_back(0.66); // y 값
    kinematics_orientation.push_back(0.00); // z 값

    setTaskSpacePath(kinematics_position, kinematics_orientation, 2.0);
    demo_count_++;
    break;


  case 8: // wait & place
    setJointSpacePath(joint_name_, present_joint_angle_, 1.0);
    gripper_value.clear();
    gripper_value.push_back(0.010);
    setToolControl(gripper_value);
    demo_count_++;
    break;

  case 9: // move up after place the box
    kinematics_position.clear();
    kinematics_orientation.clear();
    kinematics_position.push_back(0.015); // X 좌표
    kinematics_position.push_back(0.102); // Y 좌표
    kinematics_position.push_back(0.170);
    kinematics_orientation.push_back(0.74);
    kinematics_orientation.push_back(0.00);
    kinematics_orientation.push_back(0.66);
    kinematics_orientation.push_back(0.00);
    setTaskSpacePath(kinematics_position, kinematics_orientation, 2.0);
    demo_count_++;
    break;

  case 10: // initial pose
    joint_angle.clear();
    joint_angle.push_back( 0.01);
    joint_angle.push_back(-0.80);
    joint_angle.push_back( 0.00);
    joint_angle.push_back( 1.90);
    setJointSpacePath(joint_name_, joint_angle, 2.0);
    demo_count_++;
    break;

  case 11: // wait & open the gripper
    setJointSpacePath(joint_name_, present_joint_angle_, 3.0);
    gripper_value.clear();
    gripper_value.push_back(0.010);
    setToolControl(gripper_value);
    demo_count_++;
    break;

  case 12: // pick the box for marker 1
  {
    bool marker_found = false;
    kinematics_position.clear();
    kinematics_orientation.clear();
    for (int i = 0; i < ar_marker_pose.size(); i++)
    {
      if (ar_marker_pose.at(i).id == 1)
      {
        marker_found = true;
        // X, Y, Z 값을 설정
        kinematics_position.push_back(ar_marker_pose.at(i).position[0] + 0.005); // X 좌표
        kinematics_position.push_back(ar_marker_pose.at(i).position[1]);        // Y 좌표
        kinematics_position.push_back(0.033);                                  // Z 좌표 고정

        // 오리엔테이션 설정
        kinematics_orientation.push_back(0.74); // w 값
        kinematics_orientation.push_back(0.00); // x 값
        kinematics_orientation.push_back(0.66); // y 값
        kinematics_orientation.push_back(0.00); // z 값

        setTaskSpacePath(kinematics_position, kinematics_orientation, 3.0);
        demo_count_++; // 다음 단계로 진행
        break; // 찾았으므로 반복문 종료
      }
    }

    if (!marker_found)
    {
      printf("Marker 2 not detected.\n");
      demo_count_ = 10;
    }
  }
  break;

  case 13: // wait & grip
    setJointSpacePath(joint_name_, present_joint_angle_, 1.0);
    gripper_value.clear();
    gripper_value.push_back(-0.008);
    setToolControl(gripper_value);
    demo_count_++;
    break;

  case 14: // initial pose
    joint_angle.clear();
    joint_angle.push_back( 0.01);
    joint_angle.push_back(-0.80);
    joint_angle.push_back( 0.00);
    joint_angle.push_back( 1.90);
    setJointSpacePath(joint_name_, joint_angle, 2.0);
    demo_count_++;
    break;

  case 15: // place pose
    joint_angle.clear();
    joint_angle.push_back( 1.57);
    joint_angle.push_back(-0.21);
    joint_angle.push_back(-0.15);
    joint_angle.push_back( 1.89);
    setJointSpacePath(joint_name_, joint_angle, 2.0);
    demo_count_++;
    break;

  case 16: // place the box
    kinematics_position.clear();
    kinematics_orientation.clear();

    // 수정된 위치 값
    kinematics_position.push_back(0.015); // X 좌표
    kinematics_position.push_back(0.105); // Y 좌표
    kinematics_position.push_back(0.086); // Z 좌표

    // 기존 오리엔테이션 값 유지
    kinematics_orientation.push_back(0.74); // w 값
    kinematics_orientation.push_back(0.00); // x 값
    kinematics_orientation.push_back(0.66); // y 값
    kinematics_orientation.push_back(0.00); // z 값

    setTaskSpacePath(kinematics_position, kinematics_orientation, 2.0);
    demo_count_++;

    break;

  case 17: // wait & place
    setJointSpacePath(joint_name_, present_joint_angle_, 1.0);
    gripper_value.clear();
    gripper_value.push_back(0.010);
    setToolControl(gripper_value);
    demo_count_++;
    break;

  case 18: // move up after place the box
    kinematics_position.clear();
    kinematics_orientation.clear();
    kinematics_position.push_back(present_kinematic_position_.at(0));
    kinematics_position.push_back(present_kinematic_position_.at(1));
    kinematics_position.push_back(0.170);
    kinematics_orientation.push_back(0.74);
    kinematics_orientation.push_back(0.00);
    kinematics_orientation.push_back(0.66);
    kinematics_orientation.push_back(0.00);
    setTaskSpacePath(kinematics_position, kinematics_orientation, 2.0);
    demo_count_++;
    break;

  case 19: // initial pose
    joint_angle.clear();
    joint_angle.push_back( 0.01);
    joint_angle.push_back(-0.80);
    joint_angle.push_back( 0.00);
    joint_angle.push_back( 1.90);
    setJointSpacePath(joint_name_, joint_angle, 2.0);
    demo_count_++;
    break;

  case 20: // wait & open the gripper
    setJointSpacePath(joint_name_, present_joint_angle_, 3.0);
    gripper_value.clear();
    gripper_value.push_back(0.010);
    setToolControl(gripper_value);
    demo_count_++;
    break;

  case 21: // pick the box for marker 2
  {
    bool marker_found = false;
    kinematics_position.clear();
    kinematics_orientation.clear();
    for (int i = 0; i < ar_marker_pose.size(); i++)
    {
      if (ar_marker_pose.at(i).id == 2)
      {
        marker_found = true;
        // X, Y, Z 값을 설정
        kinematics_position.push_back(ar_marker_pose.at(i).position[0] + 0.005); // X 좌표
        kinematics_position.push_back(ar_marker_pose.at(i).position[1]);        // Y 좌표
        kinematics_position.push_back(0.033);                                  // Z 좌표 고정

        // 오리엔테이션 설정
        kinematics_orientation.push_back(0.74); // w 값
        kinematics_orientation.push_back(0.00); // x 값
        kinematics_orientation.push_back(0.66); // y 값
        kinematics_orientation.push_back(0.00); // z 값

        setTaskSpacePath(kinematics_position, kinematics_orientation, 3.0);
        demo_count_++; // 다음 단계로 진행
        break; // 찾았으므로 반복문 종료
      }
    }

    if (!marker_found)
    {
      printf("Marker 3 not detected.\n");
      demo_count_ = 19;
    }
  }
  break;

  case 22: // wait & grip
    setJointSpacePath(joint_name_, present_joint_angle_, 2.0);
    gripper_value.clear();
    gripper_value.push_back(-0.008);
    setToolControl(gripper_value);
    demo_count_++;
    break;

  case 23: // initial pose
    joint_angle.clear();
    joint_angle.push_back( 0.01);
    joint_angle.push_back(-0.80);
    joint_angle.push_back( 0.00);
    joint_angle.push_back( 1.90);
    setJointSpacePath(joint_name_, joint_angle, 2.0);
    demo_count_++;
    break;

  case 24: // place pose
    joint_angle.clear();
    joint_angle.push_back( 1.57);
    joint_angle.push_back(-0.21);
    joint_angle.push_back(-0.15);
    joint_angle.push_back( 1.89);
    setJointSpacePath(joint_name_, joint_angle, 2.0);
    demo_count_++;
    break;

  case 25: // place the box
    kinematics_position.clear();
    kinematics_orientation.clear();

    // 수정된 위치 값
    kinematics_position.push_back(0.019); // X 좌표
    kinematics_position.push_back(0.095); // Y 좌표
    kinematics_position.push_back(0.123); // Z 좌표

    // 기존 오리엔테이션 값 유지
    kinematics_orientation.push_back(0.74); // w 값
    kinematics_orientation.push_back(0.00); // x 값
    kinematics_orientation.push_back(0.66); // y 값
    kinematics_orientation.push_back(0.00); // z 값

    setTaskSpacePath(kinematics_position, kinematics_orientation, 2.0);
    demo_count_++;

    break;

  case 26: // wait & place
    setJointSpacePath(joint_name_, present_joint_angle_, 1.0);
    gripper_value.clear();
    gripper_value.push_back(0.010);
    setToolControl(gripper_value);
    demo_count_++;
    break;

  case 27: // move up after place the box
    kinematics_position.clear();
    kinematics_orientation.clear();
    kinematics_position.push_back(present_kinematic_position_.at(0));
    kinematics_position.push_back(present_kinematic_position_.at(1));
    kinematics_position.push_back(0.180);
    kinematics_orientation.push_back(0.74);
    kinematics_orientation.push_back(0.00);
    kinematics_orientation.push_back(0.66);
    kinematics_orientation.push_back(0.00);
    setTaskSpacePath(kinematics_position, kinematics_orientation, 2.0);
    demo_count_++;
    break;

  case 28: // home pose
    joint_angle.clear();
    joint_angle.push_back( 0.00);
    joint_angle.push_back(-1.05);
    joint_angle.push_back( 0.35);
    joint_angle.push_back( 0.70);
    setJointSpacePath(joint_name_, joint_angle, 0.01);
    demo_count_++;
    break;

    case 29: //I
    joint_angle.clear();
    joint_angle.push_back( -0.063);
    joint_angle.push_back( 0.061);
    joint_angle.push_back( -1.488);
    joint_angle.push_back( -0.012);
    setJointSpacePath(joint_name_, joint_angle, 1);
    demo_count_++;
    break;

    case 30: // wait & place
    setJointSpacePath(joint_name_, present_joint_angle_, 1.0);
    gripper_value.clear();
    gripper_value.push_back(0.010);
    setToolControl(gripper_value);
    demo_count_++;
    break;

    case 31: //R
    joint_angle.clear();
    joint_angle.push_back( -0.015);
    joint_angle.push_back( 0.030);
    joint_angle.push_back( 0.779);
    joint_angle.push_back( 1.759);
    setJointSpacePath(joint_name_, joint_angle, 1);
    demo_count_++;
    break;

    case 32: // wait & place
    setJointSpacePath(joint_name_, present_joint_angle_, 1.0);
    gripper_value.clear();
    gripper_value.push_back(0.010);
    setToolControl(gripper_value);
    demo_count_++;
    break;

    case 33: //임시
    joint_angle.clear();
    joint_angle.push_back( -0.015);
    joint_angle.push_back( -0.100);
    joint_angle.push_back( 0.779);
    joint_angle.push_back( 1.759);
    setJointSpacePath(joint_name_, joint_angle, 0.1);
    demo_count_++;
    break;

    case 34: //A
    joint_angle.clear();
    joint_angle.push_back( -0.032);
    joint_angle.push_back( 0.078);
    joint_angle.push_back( 0.894);
    joint_angle.push_back( 0.021);
    setJointSpacePath(joint_name_, joint_angle, 1);
    demo_count_++;
    break;

    case 35: // wait & place
    setJointSpacePath(joint_name_, present_joint_angle_, 1.0);
    gripper_value.clear();
    gripper_value.push_back(0.010);
    setToolControl(gripper_value);
    demo_count_++;
    break;

    case 36: //S
    joint_angle.clear();
    joint_angle.push_back( 0.000);
    joint_angle.push_back( -1.085);
    joint_angle.push_back( 0.508);
    joint_angle.push_back( -0.341);
    setJointSpacePath(joint_name_, joint_angle, 1);
    demo_count_++;
    break;

    case 37: // wait & place
     setJointSpacePath(joint_name_, present_joint_angle_, 1.0);
    gripper_value.clear();
    gripper_value.push_back(0.010);
    setToolControl(gripper_value);
    demo_count_++;
    break;

    case 38: //C
    joint_angle.clear();
    joint_angle.push_back( -0.031);
    joint_angle.push_back( -1.235);
    joint_angle.push_back( 0.032);
    joint_angle.push_back( 1.119);
    setJointSpacePath(joint_name_, joint_angle, 1);
    demo_count_++;
    break;

    case 39: // wait & place
    setJointSpacePath(joint_name_, present_joint_angle_, 1.0);
    gripper_value.clear();
    gripper_value.push_back(0.010);
    setToolControl(gripper_value);
    demo_count_++;
    break;

    case 40: // home pose
    joint_angle.clear();
    joint_angle.push_back( 0.00);
    joint_angle.push_back(-1.05);
    joint_angle.push_back( 0.35);
    joint_angle.push_back( 0.70);
    setJointSpacePath(joint_name_, joint_angle, 0.01);
    demo_count_ = 1;
    mode_state_ = DEMO_STOP;
    break;


  default:
    demo_count_++;
    break;
  }
}



void OpenManipulatorPickandPlace::printText()
{
  system("clear");

  printf("\n");
  printf("-----------------------------\n");
  printf("Pick and Place demonstration!\n");
  printf("-----------------------------\n");

  printf("1 : Home pose\n");
  printf("2 : Pick and Place demo. start\n");
  printf("3 : Pick and Place demo. Stop\n");

  printf("-----------------------------\n");

  if (mode_state_ == DEMO_START)
  {
    switch (demo_count_)
    {
    case 1: // home pose
      printf("Move home pose\n");
      break;
    case 2: // initial pose
      printf("Move initial pose\n");
      break;
    case 3:
      printf("Detecting AR marker for pick\n");
      break;
    case 4:
      printf("Grip preparation\n");
      break;
    case 5:
    case 6:
      printf("Pick the box\n");
      break;
    case 7:
    case 8:
      printf("Placing the box\n");
      break;
    case 9:
      printf("Moving up after placing\n");
      break;
    case 10:
      printf("Returning to initial pose\n");
      break;
    case 11:
      printf("Preparing for next marker\n");
      break;
    case 12:
      printf("Detecting AR marker for pick (Marker ID: 1)\n");
      break;
    case 13:
      printf("Grip preparation for Marker ID: 1\n");
      break;
    case 14:
      printf("Returning to initial pose after pick (Marker ID: 1)\n");
      break;
    case 15:
      printf("Positioning to place box (Marker ID: 1)\n");
      break;
    case 16:
      printf("Placing the box (Marker ID: 1)\n");
      break;
    case 17:
      printf("Opening gripper to release box\n");
      break;
    case 18:
      printf("Moving up after placing the box\n");
      break;
    case 19:
      printf("Returning to initial pose\n");
      break;
    case 20:
      printf("Preparing for next marker\n");
      break;
    case 21:
      printf("Detecting AR marker for pick (Marker ID: 2)\n");
      break;
    case 22:
      printf("Grip preparation for Marker ID: 2\n");
      break;
    case 23:
      printf("Returning to initial pose after pick (Marker ID: 2)\n");
      break;
    case 24:
      printf("Positioning to place box (Marker ID: 2)\n");
      break;
    case 25:
      printf("Placing the box (Marker ID: 2)\n");
      break;
    case 26:
      printf("Opening gripper to release box\n");
      break;
    case 27:
      printf("Moving up after placing the box\n");
      break;
    case 28:
      printf("Returning to home pose\n");
      break;
    case 29:
      printf("I\n");
      break;
    case 30:
      printf("wait\n");
      break;
    case 31:
      printf("R\n");
      break;
    case 32:
      printf("wait\n");
      break;
    case 33:
      printf("wait\n");
      break;
    case 34:
      printf("A\n");
      break;
    case 35:
      printf("wait\n");
      break;
    case 36:
      printf("S\n");
      break;
    case 37:
      printf("wait\n");
      break;
    case 38:
      printf("C\n");
      break;
    case 39:
        printf("C\n");
    break;
    case 40:
      printf("Returning to home pose\n");
      break;
    default:
      printf("Unknown demo state\n");
      break;
    }
  }
  else if (mode_state_ == DEMO_STOP)
  {
    printf("The end of demo\n");
  }

  printf("-----------------------------\n");
  printf("Present Joint Angle J1: %.3lf J2: %.3lf J3: %.3lf J4: %.3lf\n",
         present_joint_angle_.at(0),
         present_joint_angle_.at(1),
         present_joint_angle_.at(2),
         present_joint_angle_.at(3));
  printf("Present Tool Position: %.3lf\n", present_joint_angle_.at(4));
  printf("Present Kinematics Position X: %.3lf Y: %.3lf Z: %.3lf\n",
         present_kinematic_position_.at(0),
         present_kinematic_position_.at(1),
         present_kinematic_position_.at(2));

  if (!ar_marker_pose.empty())
  {
    printf("AR marker detected.\n");
    for (int i = 0; i < ar_marker_pose.size(); i++)
    {
      printf("ID: %d --> X: %.3lf\tY: %.3lf\tZ: %.3lf\n",
             ar_marker_pose.at(i).id,
             ar_marker_pose.at(i).position[0],
             ar_marker_pose.at(i).position[1],
             ar_marker_pose.at(i).position[2]);
    }
  }
  else
  {
    printf("No AR marker detected.\n");
  }
}



bool OpenManipulatorPickandPlace::kbhit()
{
  termios term;
  tcgetattr(0, &term);

  termios term2 = term;
  term2.c_lflag &= ~ICANON;
  tcsetattr(0, TCSANOW, &term2);

  int byteswaiting;
  ioctl(0, FIONREAD, &byteswaiting);
  tcsetattr(0, TCSANOW, &term);
  return byteswaiting > 0;
}

int main(int argc, char **argv)
{
  // Init ROS node
  ros::init(argc, argv, "open_manipulator_pick_and_place");
  ros::NodeHandle node_handle("");

  OpenManipulatorPickandPlace open_manipulator_pick_and_place;

  ros::Timer publish_timer = node_handle.createTimer(ros::Duration(0.100)/*100ms*/, &OpenManipulatorPickandPlace::publishCallback, &open_manipulator_pick_and_place);

  while (ros::ok())
  {
    ros::spinOnce();
  }
  return 0;
}
