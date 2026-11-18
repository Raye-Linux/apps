cmake_minimum_required(VERSION 3.22)

# The version every app shows in its About card is the package version,
# read from debian/changelog. project() only takes digits and dots.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(RayeVersion)
raye_version_from_changelog(${CMAKE_CURRENT_SOURCE_DIR}/debian/changelog RAYE_APPS_VERSION FALLBACK 0)
raye_numeric_version("${RAYE_APPS_VERSION}" RAYE_APPS_NUMERIC_VERSION)

project(raye-apps VERSION ${RAYE_APPS_NUMERIC_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(ECM 6.0 REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

# What every app needs. An app finds the rest itself, in its own folder,
# so a missing development package only stops the app that uses it.
find_package(Qt6 6.6 REQUIRED COMPONENTS Core)
find_package(KF6 6.0 REQUIRED COMPONENTS I18n)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()
# QML modules go under qrc:/qt/qml, where the engine looks for them.
# The policy comes with the Qml module, which the widget apps do not need.
find_package(Qt6 QUIET OPTIONAL_COMPONENTS Qml)
if(Qt6Qml_FOUND)
    qt_policy(SET QTP0001 NEW)
endif()

include(RayeApps)
include(RayeTranslations)

# raye-apps-version.h: RAYE_APPS_VERSION, for the About cards
configure_file(cmake/raye-apps-version.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/raye-apps-version.h @ONLY)
add_library(raye-apps-version INTERFACE)
target_include_directories(raye-apps-version INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/include)
add_library(Raye::Version ALIAS raye-apps-version)

# The window chrome is linked into every app, and so are its words: its
# po/<lang>/*.po fragments go into every app's catalog.
set(RAYE_SHARED_PO_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/chrome/po)

# One folder per app. PACKAGE is the Debian package its files go to;
# REQUIRES the files that package cannot ship without (its maintainer
# scripts use them). A package whose folder is HERE needs its stanza in
# debian/control; a package whose folder is NOT WRITTEN YET must not have
# one. The build checks both directions.
#
# NOT_WRITTEN_YET is the ledger of folders that do not exist yet. The
# marker makes the configure refuse a missing folder that is NOT listed,
# and refuse a listed folder that exists -- so the list can only shrink.
# See cmake/tests/test_app_ledger.cmake.
#
# PACKAGE on a marked folder is legitimate: it reserves the package name
# and keeps that package's maintainer scripts in debian/ meaningful. What
# it must NOT have is a stanza in debian/control, because dpkg builds a
# .deb for every stanza whatever this ledger says. Measured 2026-11-06 on
# bb15e0c: 13 of these 15 folders did not exist, debian/control declared
# all 13 packages anyway, and ELEVEN .deb shipped holding nothing but
# /usr/share/doc -- 1792 bytes, Installed-Size 9 -- each still pulling in
# its full Depends. raye-terminal pulled konsole-kpart and delivered no
# terminal. The only trace was one STATUS line each in an 8 MB log, and
# every build was green.
#
# So writing a folder is three things in the same commit: delete its
# marker here, and add its stanza to debian/control. The build says so if
# either is forgotten -- cmake/tests/test_control_ships_files.cmake holds
# that red, and RayeDebianInstall.cmake holds it again in a real build.
raye_add_app(chrome NOT_WRITTEN_YET)
raye_add_app(terminal       PACKAGE raye-terminal       REQUIRES bin/raye-terminal NOT_WRITTEN_YET)
raye_add_app(notepad        PACKAGE raye-notepad NOT_WRITTEN_YET)
raye_add_app(taskmanager    PACKAGE raye-taskmanager)
raye_add_app(calendar       PACKAGE raye-calendar)
raye_add_app(clock          PACKAGE raye-clock)
raye_add_app(graphics       PACKAGE raye-graphics)
raye_add_app(snip           PACKAGE raye-snip NOT_WRITTEN_YET)
raye_add_app(calculator     PACKAGE raye-calculator NOT_WRITTEN_YET)
raye_add_app(camera         PACKAGE raye-camera NOT_WRITTEN_YET)
raye_add_app(photos         PACKAGE raye-photos NOT_WRITTEN_YET)
raye_add_app(devicemanager  PACKAGE raye-devicemanager NOT_WRITTEN_YET)
raye_add_app(controlpanel   PACKAGE raye-controlpanel)
raye_add_app(regedit        PACKAGE raye-regedit)
raye_add_app(portal         PACKAGE xdg-desktop-portal-raye)
raye_add_app(notes          PACKAGE raye-notes NOT_WRITTEN_YET)
raye_add_app(gamebar        PACKAGE raye-gamebar NOT_WRITTEN_YET)
raye_add_app(hung           PACKAGE raye-killer-helper  REQUIRES ${KDE_INSTALL_LIBEXECDIR}/kwin_killer_helper NOT_WRITTEN_YET)
# names and defaults of the stock apps: data and a generator, no C++
raye_add_app(identity       PACKAGE raye-app-identity   REQUIRES libexec/raye-app-identity/raye-app-overrides)
raye_add_app(defaults       PACKAGE raye-app-identity NOT_WRITTEN_YET)
raye_write_package_list()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
