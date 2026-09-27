# CMakeList.txt : CMake project for PottersInventory, include source and define
# project specific logic here.
#

# Enable Qt automoc to process Qt meta objects for QObject/Q_SIGNALS/Q_SLOTS in .cpp/.h files.
# Do NOT enable AUTOUIC because this project is intended to use only .cpp and .h files (no .ui files).
# Must be set before add_executable() so it applies to the target when created.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

# Models/Controllers/Database are shared, UI-toolkit-agnostic, between the
# desktop app (Qt Widgets) and the REST API server (QHttpServer) below.
set(CORE_SOURCES
  "Database/Database.h" "Database/Database.cpp"
  "Models/Item.h" "Models/Item.cpp"
  "Models/Tag.h" "Models/Tag.cpp"
  "Models/Category.h" "Models/Category.cpp"
  "Models/User.h" "Models/User.cpp"
  "Models/Assignment.h" "Models/Assignment.cpp"
  "Models/ScheduleReportRow.h"
  "Models/RoleType.h" "Models/RoleType.cpp"
  "Models/Song.h" "Models/Song.cpp"
  "Models/AvailabilityMark.h" "Models/AvailabilityMark.cpp"
  "Models/NonAvailabilityRequest.h" "Models/NonAvailabilityRequest.cpp"
  "Auth/PasswordAuth.h" "Auth/PasswordAuth.cpp"
  "Controllers/ItemController.h" "Controllers/ItemController.cpp"
  "Controllers/TagController.h" "Controllers/TagController.cpp"
  "Controllers/CategoryController.h" "Controllers/CategoryController.cpp"
  "Controllers/UserController.h" "Controllers/UserController.cpp"
  "Controllers/SessionController.h" "Controllers/SessionController.cpp"
  "Controllers/AssignmentController.h" "Controllers/AssignmentController.cpp"
  "Controllers/RoleTypeController.h" "Controllers/RoleTypeController.cpp"
  "Controllers/SongController.h" "Controllers/SongController.cpp"
  "Controllers/AvailabilityController.h" "Controllers/AvailabilityController.cpp"
  "Controllers/NonAvailabilityRequestController.h" "Controllers/NonAvailabilityRequestController.cpp"
  "Vision/GroqVisionClient.h" "Vision/GroqVisionClient.cpp"
)

# --- Desktop app (Qt Widgets) ------------------------------------------------
add_executable (PottersInventory
  "PottersInventory.cpp" "PottersInventory.h"
  "Style.h" "Style.cpp"
  ${CORE_SOURCES}
  "Views/MainWindow.h" "Views/MainWindow.cpp"
  "Views/FramelessDialog.h" "Views/FramelessDialog.cpp"
  "Views/MemberStatsDialog.h" "Views/MemberStatsDialog.cpp"
  "Views/ItemFormWidget.h" "Views/ItemFormWidget.cpp"
  "Views/ItemListView.h" "Views/ItemListView.cpp"
  "Views/ItemAddDialog.h" "Views/ItemAddDialog.cpp"
  "Views/ItemEditDialog.h" "Views/ItemEditDialog.cpp"
  "Views/AdminOverviewView.h" "Views/AdminOverviewView.cpp"
  "Views/CategoryEditDialog.h" "Views/CategoryEditDialog.cpp"
  "Views/TagEditDialog.h" "Views/TagEditDialog.cpp"
  "Views/TitleBar.h" "Views/TitleBar.cpp"
  "Views/FlowLayout.h" "Views/FlowLayout.cpp"
  "Views/ClickableLabel.h" "Views/ClickableLabel.cpp"
  "Views/Badge.h" "Views/Badge.cpp"
  "Views/ItemCardWidget.h" "Views/ItemCardWidget.cpp"
  "Views/ItemDetailsDialog.h" "Views/ItemDetailsDialog.cpp"
  "Views/LoginDialog.h" "Views/LoginDialog.cpp"
  "Views/DateNavigationTab.h" "Views/DateNavigationTab.cpp"
  "Views/AssignRoleDialog.h" "Views/AssignRoleDialog.cpp"
  "Views/MemberEditDialog.h" "Views/MemberEditDialog.cpp"
  "Views/RoleTypeEditDialog.h" "Views/RoleTypeEditDialog.cpp"
  "Views/SongsView.h" "Views/SongsView.cpp"
  "Views/ReportsView.h" "Views/ReportsView.cpp"
  "Views/SongEditDialog.h" "Views/SongEditDialog.cpp"
  "Views/AvatarLoader.h" "Views/AvatarLoader.cpp"
  "Views/NonAvailabilityRequestsView.h" "Views/NonAvailabilityRequestsView.cpp"
  "Vision/GroqQueryClient.h" "Vision/GroqQueryClient.cpp"
  "Vision/LocationClient.h" "Vision/LocationClient.cpp"
)
target_include_directories(PottersInventory PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# --- REST API server (Qt HttpServer, no GUI) --------------------------------
# Same Models/Controllers/Database as the desktop app; this is the piece a
# browser-based web frontend (web/) and, later, a native mobile app both
# talk to over HTTP instead of opening a direct database connection.
add_executable (PottersInventoryServer
  ${CORE_SOURCES}
  "Server/Json.h" "Server/Json.cpp"
  "Server/AuthMiddleware.h" "Server/AuthMiddleware.cpp"
  "Server/ServerMain.cpp"
)
target_include_directories(PottersInventoryServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(PottersInventoryServer PRIVATE
  POTTERS_WEB_ROOT="${CMAKE_SOURCE_DIR}/web")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET PottersInventory PROPERTY CXX_STANDARD 20)
  set_property(TARGET PottersInventoryServer PROPERTY CXX_STANDARD 20)
endif()

# Find Qt6 Widgets + Sql (Postgres access via the QPSQL driver) + HttpServer +
# Network (outbound HTTPS call to Groq for the photo auto-fill feature, used
# by both the desktop app directly and the REST server's /api/vision route)
find_package(Qt6 COMPONENTS Widgets Sql HttpServer Network REQUIRED)
target_link_libraries(PottersInventory PRIVATE Qt6::Widgets Qt6::Sql Qt6::Network)
target_link_libraries(PottersInventoryServer PRIVATE Qt6::Sql Qt6::HttpServer Qt6::Network)

# LocationClient (desktop-only) reads the device's actual position via the
# WinRT Geolocation API (Windows Location Services) to pre-fill the Add Item
# location field -- WindowsApp.lib supplies the WinRT activation factories
# it calls into.
if (WIN32)
  target_link_libraries(PottersInventory PRIVATE WindowsApp.lib)
endif()

# --- Runtime dependency deployment -----------------------------------------
# Qt's QPSQL driver dynamically loads libpq at runtime; Qt does not ship it,
# so it's built separately via vcpkg (see .tools/vcpkg) and copied here.
# Point LIBPQ_RUNTIME_DIR at wherever your libpq.dll + dependencies live if
# it's not vcpkg's default install location.
set(LIBPQ_RUNTIME_DIR "${CMAKE_SOURCE_DIR}/.tools/vcpkg/installed/x64-windows/bin"
    CACHE PATH "Directory containing libpq.dll and its runtime dependencies")

find_program(WINDEPLOYQT_EXECUTABLE windeployqt
  HINTS "${Qt6_DIR}/../../../bin" "${Qt6Widgets_DIR}/../../../bin")

# Small app-mark icon shown in the custom title bar (TitleBar.cpp loads it
# from applicationDirPath() at runtime), deployed next to the desktop exe
# only — the headless server has no title bar to show it in.
add_custom_command(TARGET PottersInventory POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/pottershouse.jpg" "$<TARGET_FILE_DIR:PottersInventory>"
)

foreach(_target PottersInventory PottersInventoryServer)
  if (WIN32 AND EXISTS "${LIBPQ_RUNTIME_DIR}/libpq.dll")
    foreach(_dll libpq.dll libssl-3-x64.dll libcrypto-3-x64.dll lz4.dll z.dll)
      add_custom_command(TARGET ${_target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LIBPQ_RUNTIME_DIR}/${_dll}" "$<TARGET_FILE_DIR:${_target}>"
      )
    endforeach()
  endif()

  # Bundle Qt's own runtime DLLs next to the exe so it runs without Qt's bin
  # dir on PATH.
  if (WIN32 AND WINDEPLOYQT_EXECUTABLE)
    add_custom_command(TARGET ${_target} POST_BUILD
      COMMAND "${WINDEPLOYQT_EXECUTABLE}" --sql "$<TARGET_FILE:${_target}>"
    )
  endif()
endforeach()

# TODO: Add tests and install targets if needed.
