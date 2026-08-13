# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  [[CMakeFiles\dependency_probe_autogen.dir\AutogenUsed.txt]]
  [[CMakeFiles\dependency_probe_autogen.dir\ParseCache.txt]]
  [[CMakeFiles\tls401_demo_autogen.dir\AutogenUsed.txt]]
  [[CMakeFiles\tls401_demo_autogen.dir\ParseCache.txt]]
  [[CMakeFiles\tst_workflow_autogen.dir\AutogenUsed.txt]]
  [[CMakeFiles\tst_workflow_autogen.dir\ParseCache.txt]]
  "dependency_probe_autogen"
  "tls401_demo_autogen"
  "tst_workflow_autogen"
  )
endif()
