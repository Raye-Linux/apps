# Version read from debian/changelog: plain, with an epoch, broken file,
# missing file; and the digits-only form project() takes.
#   cmake -DWORK=<dir> -P test_version.cmake
include(${CMAKE_CURRENT_LIST_DIR}/../RayeVersion.cmake)

set(failures 0)
macro(expect what got want)
    if(NOT "${got}" STREQUAL "${want}")
        message(SEND_ERROR "${what}: got '${got}', want '${want}'")
        math(EXPR failures "${failures} + 1")
    endif()
endmacro()

file(MAKE_DIRECTORY ${WORK})

file(WRITE ${WORK}/plain "raye-apps (0.1~dev1) resolute; urgency=medium\n\n  * First.\n")
raye_version_from_changelog(${WORK}/plain v FALLBACK 9.9)
expect("plain" "${v}" "0.1~dev1")

file(WRITE ${WORK}/epoch "raye-apps (2:1.4.2-3) resolute; urgency=low\n")
raye_version_from_changelog(${WORK}/epoch v FALLBACK 9.9)
expect("epoch" "${v}" "2:1.4.2-3")

# only the first entry counts
file(WRITE ${WORK}/two "raye-apps (0.3) resolute; urgency=medium\n\n -- x\n\nraye-apps (0.2) resolute; urgency=medium\n")
raye_version_from_changelog(${WORK}/two v FALLBACK 9.9)
expect("first entry" "${v}" "0.3")

file(WRITE ${WORK}/broken "this is not a changelog\n")
raye_version_from_changelog(${WORK}/broken v FALLBACK 9.9)
expect("broken" "${v}" "9.9")

raye_version_from_changelog(${WORK}/does-not-exist v FALLBACK 9.9)
expect("missing" "${v}" "9.9")

raye_numeric_version("0.1~dev1" n)
expect("numeric tilde" "${n}" "0.1")
raye_numeric_version("2:1.4.2-3" n)
expect("numeric epoch" "${n}" "1.4.2")
raye_numeric_version("1.2.3.4.5" n)
expect("numeric four parts at most" "${n}" "1.2.3.4")
raye_numeric_version("7" n)
expect("numeric one part" "${n}" "7")
raye_numeric_version("dev" n)
expect("numeric none" "${n}" "0")
raye_numeric_version("" n)
expect("numeric empty" "${n}" "0")

if(failures)
    message(FATAL_ERROR "${failures} check(s) failed")
endif()
message(STATUS "version: all checks passed")
