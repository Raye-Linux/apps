# The alarm sounds, made from code at build time (make-sounds.py). The
# names are the ones alarms.json keeps; ClockModel lists them in the
# same order.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(RAYE_CLOCK_SOUNDS glow chimes marimba pulse dawn)
# under share/ of the build, so XDG_DATA_DIRS can point a test run at them
set(out "${CMAKE_BINARY_DIR}/share/raye-clock/sounds")
set(wavs "")
foreach(name IN LISTS RAYE_CLOCK_SOUNDS)
    list(APPEND wavs "${out}/${name}.wav")
endforeach()
add_custom_command(
    OUTPUT ${wavs}
    COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/make-sounds.py "${out}" ${RAYE_CLOCK_SOUNDS}
    DEPENDS make-sounds.py
    COMMENT "Alarm sounds"
    VERBATIM)
add_custom_target(raye-clock-sounds ALL DEPENDS ${wavs})
install(FILES ${wavs} DESTINATION ${KDE_INSTALL_DATADIR}/raye-clock/sounds)
