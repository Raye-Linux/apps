# The apps show the package version in their About card. It is read from
# the first entry of debian/changelog, so the window and dpkg agree.
#
#   raye_version_from_changelog(<changelog> <out-var> FALLBACK <version>)

function(raye_version_from_changelog changelog out)
    cmake_parse_arguments(ARG "" "FALLBACK" "" ${ARGN})
    set(version "${ARG_FALLBACK}")
    if(EXISTS "${changelog}")
        file(STRINGS "${changelog}" first LIMIT_COUNT 1)
        # "raye-apps (0.1~dev1) resolute; urgency=medium"
        if(first MATCHES "^[a-z0-9][a-z0-9.+-]* \\(([^ ()]+)\\) ")
            set(version "${CMAKE_MATCH_1}")
        endif()
    endif()
    set(${out} "${version}" PARENT_SCOPE)
endfunction()

# The part of a package version project(VERSION) accepts: no epoch, digits
# and dots up to four parts, "0" when there is none.
#   raye_numeric_version(<version> <out-var>)

function(raye_numeric_version version out)
    string(REGEX REPLACE "^[0-9]+:" "" rest "${version}")
    set(numeric "0")
    if(rest MATCHES "^[0-9]+(\\.[0-9]+)?(\\.[0-9]+)?(\\.[0-9]+)?")
        set(numeric "${CMAKE_MATCH_0}")
    endif()
    set(${out} "${numeric}" PARENT_SCOPE)
endfunction()
