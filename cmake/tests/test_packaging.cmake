# End to end on a small tree shaped like the apps repo: folders skipped
# while absent, components, catalogs merged from fragments, QtTest
# helper, and the Debian install with its checks.
#   cmake -DWORK=<dir> -DRAYE_CMAKE_DIR=<cmake/> -DGENERATOR=<gen> [-DWITH_QT=ON] -P test_packaging.cmake

find_program(MSGUNFMT msgunfmt REQUIRED)

set(failed FALSE)
macro(fail text)
    message(SEND_ERROR "${text}")
    set(failed TRUE)
endmacro()

# ---- the fixture tree ------------------------------------------------

function(write_tree root variant)
    file(REMOVE_RECURSE ${root})
    set(qt "")
    if(WITH_QT AND variant STREQUAL "good")
        set(qt "find_package(Qt6 REQUIRED COMPONENTS Core Test)")
    endif()
    set(alpha_requires "bin/raye-alpha")
    if(variant STREQUAL "missing-required")
        set(alpha_requires "bin/raye-alpha libexec/raye-alpha-helper")
    endif()
    file(WRITE ${root}/CMakeLists.txt "
cmake_minimum_required(VERSION 3.22)
project(fixture LANGUAGES CXX)
list(APPEND CMAKE_MODULE_PATH ${RAYE_CMAKE_DIR})
include(RayeApps)
include(RayeTranslations)
enable_testing()
${qt}
set(RAYE_SHARED_PO_DIRS \${CMAKE_CURRENT_SOURCE_DIR}/shared/po)
raye_add_app(shared)
raye_add_app(alpha PACKAGE raye-alpha REQUIRES ${alpha_requires})
raye_add_app(beta PACKAGE raye-beta REQUIRES bin/raye-beta NOT_WRITTEN_YET)
raye_add_app(gamma PACKAGE raye-alpha)
raye_write_package_list()
")
    # shared code: only translations, installs nothing
    file(WRITE ${root}/shared/CMakeLists.txt "
if(DEFINED RAYE_APP_PACKAGE)
    message(FATAL_ERROR \"shared sees a package\")
endif()
")
    file(WRITE ${root}/shared/po/fr/chrome.po "msgid \"\"\nmsgstr \"Content-Type: text/plain; charset=UTF-8\\n\"\n\nmsgid \"Minimise\"\nmsgstr \"Réduire\"\n")

    set(alpha "
if(NOT RAYE_APP_DOMAIN STREQUAL \"raye-alpha\")
    message(FATAL_ERROR \"wrong domain \${RAYE_APP_DOMAIN}\")
endif()
install(PROGRAMS raye-alpha DESTINATION bin)
raye_app_translations()
")
    if(WITH_QT AND variant STREQUAL "good")
        string(APPEND alpha "raye_add_qtest(smoke SOURCES test_smoke.cpp)\n")
        file(WRITE ${root}/alpha/test_smoke.cpp "
#include <QTest>
class Smoke : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void runs() { QCOMPARE(qgetenv(\"QT_QPA_PLATFORM\"), QByteArray(\"offscreen\")); }
};
QTEST_GUILESS_MAIN(Smoke)
#include \"test_smoke.moc\"
")
        string(PREPEND alpha "set(CMAKE_AUTOMOC ON)\n")
    endif()
    file(WRITE ${root}/alpha/CMakeLists.txt "${alpha}")
    file(WRITE ${root}/alpha/raye-alpha "#!/bin/sh\necho alpha\n")
    file(WRITE ${root}/alpha/po/fr/core.po "msgid \"\"\nmsgstr \"Content-Type: text/plain; charset=UTF-8\\nPO-Revision-Date: 2026-10-01\\n\"\n\nmsgid \"New tab\"\nmsgstr \"Nouvel onglet\"\n\nmsgid \"Close\"\nmsgstr \"Fermer\"\n")
    set(second "msgid \"Close\"\nmsgstr \"Fermer\"\n")
    if(variant STREQUAL "conflict")
        set(second "msgid \"Close\"\nmsgstr \"Quitter\"\n")
    endif()
    file(WRITE ${root}/alpha/po/fr/settings.po "msgid \"\"\nmsgstr \"Content-Type: text/plain; charset=UTF-8\\nPO-Revision-Date: 2026-10-02\\n\"\n\nmsgid \"Settings\"\nmsgstr \"Paramètres\"\n\n${second}")

    set(component "")
    if(variant STREQUAL "stray")
        set(component "COMPONENT raye-alhpa")
    endif()
    file(WRITE ${root}/gamma/CMakeLists.txt "install(FILES data.txt DESTINATION share/raye-alpha ${component})\n")
    file(WRITE ${root}/gamma/data.txt "data\n")
    # beta has no CMakeLists.txt: an app folder still being written, and
    # so marked NOT_WRITTEN_YET above. cmake/tests/test_app_ledger.cmake
    # is what proves the marker is required and cannot go stale.
    file(WRITE ${root}/beta/README "later\n")

    # the packages debian/control knows, as dpkg reads them
    # raye-beta's folder is not written, so it has NO stanza. The stanza is
    # what makes dpkg build a .deb, and that .deb would hold nothing but
    # /usr/share/doc with raye-beta's full Depends still pulled in.
    set(stanzas "Package: raye-alpha\nArchitecture: any\n")
    if(variant STREQUAL "control-missing")
        set(stanzas "")
    elseif(variant STREQUAL "control-extra")
        string(APPEND stanzas "\nPackage: raye-delta\nArchitecture: any\n")
    elseif(variant STREQUAL "control-empty")
        # exactly the shape this repository shipped until 2026-11-06
        string(APPEND stanzas "\nPackage: raye-beta\nArchitecture: all\n")
    endif()
    file(WRITE ${root}/debian/control "Source: fixture\nBuild-Depends: cmake,\n               gettext\n\n${stanzas}")
endfunction()

function(configure_and_build root build out_ok out_log)
    file(REMOVE_RECURSE ${build})
    execute_process(
        COMMAND ${CMAKE_COMMAND} -S ${root} -B ${build} -G "${GENERATOR}" -DCMAKE_INSTALL_PREFIX=/usr
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(rc EQUAL 0)
        execute_process(COMMAND ${CMAKE_COMMAND} --build ${build}
            RESULT_VARIABLE rc OUTPUT_VARIABLE out2 ERROR_VARIABLE err2)
        string(APPEND out "${out2}")
        string(APPEND err "${err2}")
    endif()
    if(rc EQUAL 0)
        set(${out_ok} TRUE PARENT_SCOPE)
    else()
        set(${out_ok} FALSE PARENT_SCOPE)
    endif()
    set(${out_log} "${out}\n${err}" PARENT_SCOPE)
endfunction()

function(debian_install build debian control out_ok out_log)
    file(REMOVE_RECURSE ${debian})
    execute_process(
        COMMAND ${CMAKE_COMMAND} -DBUILD_DIR=${build} -DDEBIAN_DIR=${debian}
                -DCONTROL=${control}
                -P ${RAYE_CMAKE_DIR}/RayeDebianInstall.cmake
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(rc EQUAL 0)
        set(${out_ok} TRUE PARENT_SCOPE)
    else()
        set(${out_ok} FALSE PARENT_SCOPE)
    endif()
    set(${out_log} "${out}\n${err}" PARENT_SCOPE)
endfunction()

# ---- good tree -------------------------------------------------------

set(root ${WORK}/good-src)
set(build ${WORK}/good-build)
set(debian ${WORK}/good-debian)
write_tree(${root} good)
configure_and_build(${root} ${build} ok log)
if(NOT ok)
    message(FATAL_ERROR "good tree does not build:\n${log}")
endif()
string(FIND "${log}" "raye-beta is not built" at)
if(at EQUAL -1)
    fail("a folder on the NOT_WRITTEN_YET ledger is not reported with the package name it reserves")
endif()

file(READ ${build}/raye-packages.list list)
set(want "package raye-alpha present\npackage raye-beta absent\nrequire raye-alpha /usr/bin/raye-alpha\n")
if(NOT list STREQUAL want)
    fail("package list:\n${list}\nwanted:\n${want}")
endif()

debian_install(${build} ${debian} ${root}/debian/control ok log)
if(NOT ok)
    fail("Debian install of the good tree failed:\n${log}")
endif()
foreach(f bin/raye-alpha share/raye-alpha/data.txt share/locale/fr/LC_MESSAGES/raye-alpha.mo)
    if(NOT EXISTS ${debian}/raye-alpha/usr/${f})
        fail("raye-alpha lacks /usr/${f}")
    endif()
endforeach()
if(EXISTS ${debian}/raye-beta)
    file(GLOB_RECURSE beta_files ${debian}/raye-beta/*)
    if(beta_files)
        fail("raye-beta should be empty: ${beta_files}")
    endif()
endif()

# the catalog holds the app's two fragments and the shared one
execute_process(COMMAND ${MSGUNFMT} ${debian}/raye-alpha/usr/share/locale/fr/LC_MESSAGES/raye-alpha.mo
    OUTPUT_VARIABLE catalog RESULT_VARIABLE rc)
foreach(msg "Nouvel onglet" "Fermer" "Paramètres" "Réduire")
    string(FIND "${catalog}" "${msg}" at)
    if(at EQUAL -1)
        fail("catalog lacks ${msg}:\n${catalog}")
    endif()
endforeach()

if(WITH_QT)
    execute_process(COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${build} --output-on-failure
        RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    string(FIND "${out}" "raye-alpha/smoke" at)
    if(NOT rc EQUAL 0 OR at EQUAL -1)
        fail("raye_add_qtest test did not run or failed:\n${out}\n${err}")
    endif()
endif()

# ---- broken trees: each must stop the build and say why -------------

foreach(case stray missing-required conflict control-missing control-extra control-empty)
    set(root ${WORK}/${case}-src)
    set(build ${WORK}/${case}-build)
    write_tree(${root} ${case})
    configure_and_build(${root} ${build} ok log)
    if(case STREQUAL "conflict")
        set(want_text "translated differently in two fragments")
        if(ok)
            fail("conflict: the build should have failed")
            continue()
        endif()
    else()
        if(NOT ok)
            fail("${case}: does not build:\n${log}")
            continue()
        endif()
        debian_install(${build} ${WORK}/${case}-debian ${root}/debian/control ok log)
        if(ok)
            fail("${case}: the Debian install should have failed")
            continue()
        endif()
        if(case STREQUAL "stray")
            set(want_text "/usr/share/raye-alpha/data.txt is installed but in no package")
        elseif(case STREQUAL "control-missing")
            set(want_text "raye-alpha is named by a folder that IS in the tree and has no stanza in debian/control")
        elseif(case STREQUAL "control-extra")
            set(want_text "debian/control has raye-delta, which no app folder in CMakeLists.txt names")
        elseif(case STREQUAL "control-empty")
            set(want_text "debian/control declares raye-beta while its folder is not written")
        else()
            set(want_text "raye-alpha must ship /usr/libexec/raye-alpha-helper")
        endif()
    endif()
    string(FIND "${log}" "${want_text}" at)
    if(at EQUAL -1)
        fail("${case}: the error does not say \"${want_text}\":\n${log}")
    endif()
endforeach()

if(failed)
    message(FATAL_ERROR "packaging: some checks failed")
endif()
message(STATUS "packaging: all checks passed")
