# Dependencies of mastersolution tests:

# DIR will be provided by the calling file.

set(SOURCES
  ${DIR}/test/maximum_principle_test.cc
)

set(LIBRARIES
  Eigen3::Eigen
  GTest::gtest_main
)
