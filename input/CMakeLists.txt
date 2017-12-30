# Copy example files into the shared directory.
install(DIRECTORY
  Baobab
  Chinese
  CEA9601
  ne574
  SmallTree
  BSCU
  Theatre
  TwoTrain
  ThreeMotor
  Lift
  HIPPS
  EventTrees
  DESTINATION share/scram/input
  COMPONENT examples
  FILES_MATCHING PATTERN "*.xml"
  PATTERN "RiskMan*" EXCLUDE
  )
