INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/gui
  ${CMAKE_CURRENT_SOURCE_DIR}/gui/images
  ${CMAKE_CURRENT_SOURCE_DIR}/plugins
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/wxmathplot-0.03
  ${OpenCV_INCLUDE_DIRS}
  ${WXWINDOWS_INCLUDE_DIR}
)

SET( CELLTRACK_SRC_FILES
  CaptureManager.cpp
  ImagePlus.cpp
  MyCapture.cpp
  PluginBase.cpp
  CustomEvents.cpp
  MyApp.cpp
  MyFrame.cpp
  Preferences.cpp
  cvSnakeTrack.cpp
  MyCanvas.cpp
  PictureCanvas.cpp
  Util.cpp

  gui/wxTextCtrl_double.cpp
  gui/Gui.cpp

  lib/wxmathplot-0.03/mathplot.cpp
  lib/wxmathplot-0.03/mpFXVector.cpp

  plugins/CamShiftPlugin.cpp
  plugins/FindFeaturesPlugin.cpp
  plugins/ResizeMovieSidebar.cpp
  plugins/CamShiftSidebar.cpp
  plugins/FindFeaturesSidebar.cpp
  plugins/ResizePlugin.cpp
  plugins/CombinedTrackingPlugin.cpp
  plugins/ImproveContoursPlugin.cpp
  plugins/ResizeSidebar.cpp
  plugins/CombinedTrackingSidebar.cpp
  plugins/ImproveContoursSidebar.cpp
  plugins/CropPlugin.cpp
  plugins/LKContoursPlugin.cpp
  plugins/SmoothPlugin.cpp
  plugins/CropSidebar.cpp
  plugins/LKContoursSidebar.cpp
  plugins/SmoothSidebar.cpp
  plugins/EditContoursPlugin.cpp
  plugins/MatchTemplatePlugin.cpp
  plugins/SubtractBackgroundPlugin.cpp
  plugins/EditContoursSidebar.cpp
  plugins/MatchTemplateSidebar.cpp
  plugins/SubtractBackgroundSidebar.cpp
  plugins/FilterContoursPlugin.cpp
  plugins/NormalizeContoursPlugin.cpp
  plugins/TrackContoursPlugin.cpp
  plugins/FilterContoursSidebar.cpp
  plugins/NormalizeContoursSidebar.cpp
  plugins/TrackContoursSidebar.cpp
  plugins/FindContoursPlugin.cpp
  plugins/FindContoursSidebar.cpp
  plugins/ResizeMoviePlugin.cpp
)

ADD_EXECUTABLE(CellTrack
  ${CELLTRACK_SRC_FILES}
)
TARGET_LINK_LIBRARIES(CellTrack
  ${OpenCV_LIBRARIES}
  ${WXWINDOWS_LINK_DIRECTORIES}
)

