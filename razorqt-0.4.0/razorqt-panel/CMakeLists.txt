project( razor-panel )
cmake_minimum_required( VERSION 2.6 )

if (NOT CMAKE_BUILD_TYPE)
    set ( CMAKE_BUILD_TYPE Release )
endif (NOT CMAKE_BUILD_TYPE)


macro( setByDefault VAR_NAME VAR_VALUE )
  if (NOT DEFINED ${VAR_NAME})
    set (${VAR_NAME} ${VAR_VALUE})
  endif (NOT DEFINED ${VAR_NAME})
endmacro()


# *******************************************************************
# What plugins will be built, by default.
# You can enable/disable building of the plugin using cmake options.
#    cmake -DCLOCK_PLUGIN=Yes .. # Enable clock plugin
#    cmake -DCLOCK_PLUGIN=No ..  # Disable clock plugin

setByDefault(CLOCK_PLUGIN       Yes )
setByDefault(DESKTOPSWITCH_PLUGIN  Yes )
setByDefault(QUICKLAUNCH_PLUGIN Yes )
setByDefault(TRAY_PLUGIN        Yes )
setByDefault(MAINMENU_PLUGIN    Yes )
setByDefault(TASKBAR_PLUGIN     Yes )
setByDefault(SCREENSAVER_PLUGIN  Yes )
setByDefault(SHOWDESKTOP_PLUGIN Yes)
setByDefault(MOUNT_PLUGIN Yes)

# *******************************************************************



include(RazorLibSuffix)
set (X11_INCLUDE_DIR /opt/gi/include/)


add_definitions ( -Wall )
find_package ( Qt4 REQUIRED )
#find_package ( X11 REQUIRED )
include ( ${QT_USE_FILE} )
include_directories (
	${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
	${QT_QTCORE_INCLUDE_DIR} ${QT_QTGUI_INCLUDE_DIR} ${QT_QTXML_INCLUDE_DIR} ${QT_QTDBUS_INCLUDE_DIR}
    ${X11_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/libraries/
	src)

set (PLUGIN_DIR ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/razor-panel/)
add_definitions(-DPLUGIN_DIR=\"${PLUGIN_DIR}\")
message(STATUS "Panel plugins location: ${PLUGIN_DIR}")


add_subdirectory(panel)


set(ENABLED_PLUGINS)

if (CLOCK_PLUGIN)
  set(ENABLED_PLUGINS ${ENABLED_PLUGINS}  "Clock")
  add_subdirectory( plugin-clock )
endif (CLOCK_PLUGIN)

if (DESKTOPSWITCH_PLUGIN)
  set(ENABLED_PLUGINS ${ENABLED_PLUGINS} "DesktopSwitch")
  add_subdirectory( plugin-desktopswitch )
endif (DESKTOPSWITCH_PLUGIN)

if (QUICKLAUNCH_PLUGIN)
  set(ENABLED_PLUGINS ${ENABLED_PLUGINS}  "Quicklaunch")
  add_subdirectory( plugin-quicklaunch )
endif (QUICKLAUNCH_PLUGIN)

if (TRAY_PLUGIN)
  set(ENABLED_PLUGINS ${ENABLED_PLUGINS}  "System Tray")
  add_subdirectory( plugin-tray )
endif (TRAY_PLUGIN)

if (MAINMENU_PLUGIN)
  set(ENABLED_PLUGINS ${ENABLED_PLUGINS}  "Main menu")
  add_subdirectory( plugin-mainmenu )
endif (MAINMENU_PLUGIN)

if (MOUNT_PLUGIN)
  if (NOT UDEV_STABLE)
    message(WARNING "lubudev is not stable - panel mount plugin is disabled")
  else (NOT UDEV_STABLE)
    set(ENABLED_PLUGINS ${ENABLED_PLUGINS}  "Mount")
    add_subdirectory( plugin-mount )
  endif (NOT UDEV_STABLE)
endif (MOUNT_PLUGIN)

if (TASKBAR_PLUGIN)
  set(ENABLED_PLUGINS ${ENABLED_PLUGINS}  "Taskbar")
  add_subdirectory( plugin-taskbar )
endif (TASKBAR_PLUGIN)

if (SCREENSAVER_PLUGIN)
    set(ENABLED_PLUGINS ${ENABLED_PLUGINS}   "Screensaver")
    add_subdirectory( plugin-screensaver )
endif (SCREENSAVER_PLUGIN)

if (SHOWDESKTOP_PLUGIN)
    set(ENABLED_PLUGINS ${ENABLED_PLUGINS}   "ShowDesktop")
    add_subdirectory(plugin-showdesktop)
endif (SHOWDESKTOP_PLUGIN)

message(STATUS  "**************** The following plugins will be built ****************")
foreach (PLUGIN_STR ${ENABLED_PLUGINS})
  message(STATUS   "  ${PLUGIN_STR}")
endforeach()
message(STATUS  "*********************************************************************")

