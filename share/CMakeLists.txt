install(FILES
  project.rng
  input.rng
  report.rng
  DESTINATION share/scram
  COMPONENT scram
  )

if(BUILD_GUI)
  install(FILES
    gui.rng
    DESTINATION share/scram
    COMPONENT gui
  )
endif()
