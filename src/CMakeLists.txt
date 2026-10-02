# CMakeList.txt : CMake project for PottersPortal, include source and define
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
  "Models/MemberColors.h" "Models/MemberColors.cpp"
  "Models/Duty.h" "Models/Duty.cpp"
  "Models/ScheduleReportRow.h"
  "Models/DutyType.h" "Models/DutyType.cpp"
  "Models/Team.h" "Models/Team.cpp"
  "Models/Feedback.h" "Models/Feedback.cpp"
  "Models/AccessRights.h" "Models/AccessRights.cpp"
  "Models/Song.h" "Models/Song.cpp"
  "Models/AvailabilityMark.h" "Models/AvailabilityMark.cpp"
  "Models/NonAvailabilityRequest.h" "Models/NonAvailabilityRequest.cpp"
  "Auth/PasswordAuth.h" "Auth/PasswordAuth.cpp"
  "Controllers/ItemController.h" "Controllers/ItemController.cpp"
  "Controllers/TagController.h" "Controllers/TagController.cpp"
  "Controllers/CategoryController.h" "Controllers/CategoryController.cpp"
  "Controllers/UserController.h" "Controllers/UserController.cpp"
  "Controllers/SessionController.h" "Controllers/SessionController.cpp"
  "Controllers/DutyController.h" "Controllers/DutyController.cpp"
  "Controllers/DutyTypeController.h" "Controllers/DutyTypeController.cpp"
  "Controllers/TeamController.h" "Controllers/TeamController.cpp"
  "Controllers/FeedbackController.h" "Controllers/FeedbackController.cpp"
  "Controllers/AccessController.h" "Controllers/AccessController.cpp"
  "Controllers/SongController.h" "Controllers/SongController.cpp"
  "Controllers/AvailabilityController.h" "Controllers/AvailabilityController.cpp"
  "Controllers/NonAvailabilityRequestController.h" "Controllers/NonAvailabilityRequestController.cpp"
  "Vision/GroqVisionClient.h" "Vision/GroqVisionClient.cpp"
)

# --- Desktop app (Qt Widgets) ------------------------------------------------
add_executable (PottersPortal
  "Portal.cpp" "Portal.h"
  "Style.h" "Style.cpp"
  "Language.h" "Language.cpp"
  "PottersPortal.rc"
  ${CORE_SOURCES}
  "Views/MainWindow.h" "Views/MainWindow.cpp"
  "Views/FramelessDialog.h" "Views/FramelessDialog.cpp"
  "Views/MessageDialog.h" "Views/MessageDialog.cpp"
  "Views/MemberStatsDialog.h" "Views/MemberStatsDialog.cpp"
  "Views/ItemFormWidget.h" "Views/ItemFormWidget.cpp"
  "Views/ItemListView.h" "Views/ItemListView.cpp"
  "Views/ItemAddDialog.h" "Views/ItemAddDialog.cpp"
  "Views/ItemEditDialog.h" "Views/ItemEditDialog.cpp"
  "Views/AdminOverviewView.h" "Views/AdminOverviewView.cpp"
  "Views/CategoryEditDialog.h" "Views/CategoryEditDialog.cpp"
  "Views/TagEditDialog.h" "Views/TagEditDialog.cpp"
  "Views/TitleBar.h" "Views/TitleBar.cpp"
  "Views/Sidebar.h" "Views/Sidebar.cpp"
  "Views/ActionBar.h" "Views/ActionBar.cpp"
  "Views/SettingsView.h" "Views/SettingsView.cpp"
  "Views/FlowLayout.h" "Views/FlowLayout.cpp"
  "Views/ClickableLabel.h" "Views/ClickableLabel.cpp"
  "Views/Badge.h" "Views/Badge.cpp"
  "Views/ItemCardWidget.h" "Views/ItemCardWidget.cpp"
  "Views/ItemDetailsDialog.h" "Views/ItemDetailsDialog.cpp"
  "Views/LoginDialog.h" "Views/LoginDialog.cpp"
  "Views/ChangePasswordDialog.h" "Views/ChangePasswordDialog.cpp"
  "Views/AdminContact.h"
  "Mail/SmtpMail.h" "Mail/SmtpMail.cpp"
  "Views/ScheduleTab.h" "Views/ScheduleTab.cpp"
  "Views/AssignDutyDialog.h" "Views/AssignDutyDialog.cpp"
  "Views/AddToScheduleDialog.h" "Views/AddToScheduleDialog.cpp"
  "Views/MemberEditDialog.h" "Views/MemberEditDialog.cpp"
  "Views/DutyTypeEditDialog.h" "Views/DutyTypeEditDialog.cpp"
  "Views/TeamEditDialog.h" "Views/TeamEditDialog.cpp"
  "Views/FeedbackView.h" "Views/FeedbackView.cpp"
  "Views/AccessRightsDialog.h" "Views/AccessRightsDialog.cpp"
  "Views/ElidedLabel.h" "Views/ElidedLabel.cpp"
  "Views/MemberSundayDialog.h" "Views/MemberSundayDialog.cpp"
  "Views/SongsView.h" "Views/SongsView.cpp"
  "Views/ReportsView.h" "Views/ReportsView.cpp"
  "Views/MemberPickerField.h" "Views/MemberPickerField.cpp"
  "Views/SongEditDialog.h" "Views/SongEditDialog.cpp"
  "Views/MemberBadge.h" "Views/MemberBadge.cpp"
  "Views/MemberColorPicker.h" "Views/MemberColorPicker.cpp"
  "Views/SuggestLineEdit.h" "Views/SuggestLineEdit.cpp"
  "Vision/GroqQueryClient.h" "Vision/GroqQueryClient.cpp"
  "Vision/LocationClient.h" "Vision/LocationClient.cpp"
)
target_include_directories(PottersPortal PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# --- REST API server (Qt HttpServer, no GUI) --------------------------------
# Same Models/Controllers/Database as the desktop app; this is the piece the
# mobile app (mobile/) talks to over HTTP instead of opening a direct
# database connection.
add_executable (PottersPortalServer
  ${CORE_SOURCES}
  "Server/Json.h" "Server/Json.cpp"
  "Server/AuthMiddleware.h" "Server/AuthMiddleware.cpp"
  "Server/ServerMain.cpp"
)
target_include_directories(PottersPortalServer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET PottersPortal PROPERTY CXX_STANDARD 20)
  set_property(TARGET PottersPortalServer PROPERTY CXX_STANDARD 20)
endif()

# Find Qt6 Widgets + Sql (Postgres access via the QPSQL driver) + HttpServer +
# Network (outbound HTTPS call to Groq for the photo auto-fill feature, used
# by both the desktop app directly and the REST server's /api/vision route)
find_package(Qt6 COMPONENTS Widgets Sql HttpServer Network LinguistTools REQUIRED)
target_link_libraries(PottersPortal PRIVATE Qt6::Widgets Qt6::Sql Qt6::Network)
target_link_libraries(PottersPortalServer PRIVATE Qt6::Sql Qt6::HttpServer Qt6::Network)

# French UI text (Settings > Language). The .ts file holds every tr() string
# with its French version; it's compiled to a .qm and built into the desktop
# app as :/i18n/pottersportal_fr.qm (see Language.cpp). After changing UI
# text, build the "update_translations" target to pick up new strings, then
# fill in their French in the .ts file.
qt_add_translations(PottersPortal
  TS_FILES "${CMAKE_CURRENT_SOURCE_DIR}/translations/pottersportal_fr.ts"
  LUPDATE_OPTIONS -no-obsolete -locations none)

# LocationClient (desktop-only) reads the device's actual position via the
# WinRT Geolocation API (Windows Location Services) to pre-fill the Add Item
# location field -- WindowsApp.lib supplies the WinRT activation factories
# it calls into.
if (WIN32)
  target_link_libraries(PottersPortal PRIVATE WindowsApp.lib)
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
add_custom_command(TARGET PottersPortal POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/pottershouse.jpg" "$<TARGET_FILE_DIR:PottersPortal>"
)

foreach(_target PottersPortal PottersPortalServer)
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
