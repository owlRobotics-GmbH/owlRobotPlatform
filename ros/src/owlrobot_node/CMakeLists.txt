cmake_minimum_required(VERSION 2.8.3)
project(owlrobot_node)

find_package(catkin REQUIRED)
catkin_package(DEPENDS)

install(DIRECTORY launch
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
)

install(DIRECTORY src
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
)