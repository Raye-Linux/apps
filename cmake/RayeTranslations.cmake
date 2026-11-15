# French (and later other) catalogs of an app, made of one fragment per
# work package: <app>/po/<lang>/<fragment>.po. The fragments of a
# language are merged with msgcat into <domain>.po, compiled with msgfmt
# and installed as share/locale/<lang>/LC_MESSAGES/<domain>.mo.
#
#   raye_app_translations([DOMAIN <domain>] [PO_DIR <dir>] [SHARED <dir>...])
#
# DOMAIN defaults to RAYE_APP_DOMAIN, PO_DIR to the calling folder's po/.
# SHARED adds the fragments of code linked into every app (the window
# chrome); it defaults to RAYE_SHARED_PO_DIRS. Shared code must not
# define TRANSLATION_DOMAIN: its i18n() calls then use the app's domain.
#
# The same message translated two different ways in two fragments stops
# the build and names the message.

set(RAYE_CHECK_PO_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/RayeCheckPo.cmake)

function(raye_app_translations)
    cmake_parse_arguments(ARG "" "DOMAIN;PO_DIR" "SHARED" ${ARGN})
    if(NOT ARG_DOMAIN)
        set(ARG_DOMAIN "${RAYE_APP_DOMAIN}")
    endif()
    if(NOT ARG_DOMAIN)
        message(FATAL_ERROR "raye_app_translations: no DOMAIN, and not inside raye_add_app()")
    endif()
    if(NOT ARG_PO_DIR)
        set(ARG_PO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/po")
    endif()
    if(NOT DEFINED ARG_SHARED)
        set(ARG_SHARED ${RAYE_SHARED_PO_DIRS})
    endif()

    find_program(RAYE_MSGCAT msgcat REQUIRED)
    find_program(RAYE_MSGFMT msgfmt REQUIRED)

    # GNUInstallDirs can leave an empty cache entry behind (Qt's own
    # config files include it): an empty value is no value
    if(NOT "${KDE_INSTALL_LOCALEDIR}" STREQUAL "")
        set(localedir "${KDE_INSTALL_LOCALEDIR}")
    elseif(NOT "${CMAKE_INSTALL_LOCALEDIR}" STREQUAL "")
        set(localedir "${CMAKE_INSTALL_LOCALEDIR}")
    else()
        set(localedir "share/locale")
    endif()

    file(GLOB langs RELATIVE "${ARG_PO_DIR}" "${ARG_PO_DIR}/*")
    list(SORT langs)
    set(outputs "")
    foreach(lang IN LISTS langs)
        if(NOT IS_DIRECTORY "${ARG_PO_DIR}/${lang}")
            continue()
        endif()
        file(GLOB fragments "${ARG_PO_DIR}/${lang}/*.po")
        list(SORT fragments)
        foreach(dir IN LISTS ARG_SHARED)
            file(GLOB shared "${dir}/${lang}/*.po")
            list(SORT shared)
            list(APPEND fragments ${shared})
        endforeach()
        if(NOT fragments)
            continue()
        endif()

        set(work "${CMAKE_CURRENT_BINARY_DIR}/po/${lang}")
        set(mo "${work}/${ARG_DOMAIN}.mo")
        add_custom_command(
            OUTPUT "${mo}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${work}"
            # without --use-first, msgcat marks every message translated
            # two ways; the check fails on those
            COMMAND ${RAYE_MSGCAT} -o "${work}/all.po" ${fragments}
            COMMAND ${CMAKE_COMMAND} -DPO_FILE=${work}/all.po
                    -DDOMAIN=${ARG_DOMAIN} -DLANG=${lang} -P ${RAYE_CHECK_PO_SCRIPT}
            COMMAND ${RAYE_MSGCAT} --use-first -o "${work}/${ARG_DOMAIN}.po" ${fragments}
            COMMAND ${RAYE_MSGFMT} --check -o "${mo}" "${work}/${ARG_DOMAIN}.po"
            DEPENDS ${fragments} ${RAYE_CHECK_PO_SCRIPT}
            COMMENT "Catalog ${lang}/${ARG_DOMAIN}.mo"
            VERBATIM)
        list(APPEND outputs "${mo}")
        install(FILES "${mo}" DESTINATION "${localedir}/${lang}/LC_MESSAGES")
    endforeach()

    if(outputs)
        add_custom_target(${ARG_DOMAIN}-translations ALL DEPENDS ${outputs})
    else()
        message(STATUS "Raye apps: no catalog for ${ARG_DOMAIN} yet")
    endif()
endfunction()
