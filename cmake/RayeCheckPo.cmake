# Fails when msgcat found a message translated two different ways in two
# fragments. The header entry (empty msgid) differs between fragments
# all the time and is not checked.
#   cmake -DPO_FILE=<merged.po> -DDOMAIN=<domain> -DLANG=<lang> -P RayeCheckPo.cmake

if(NOT EXISTS "${PO_FILE}")
    message(FATAL_ERROR "RayeCheckPo: ${PO_FILE} does not exist")
endif()

# one list item per line; ';' and brackets would split or glue items,
# so they are swapped for markers and put back in the messages shown
file(READ "${PO_FILE}" content)
string(REPLACE ";" "@SEMI@" content "${content}")
string(REPLACE "[" "@LB@" content "${content}")
string(REPLACE "]" "@RB@" content "${content}")
string(REPLACE "\r" "" content "${content}")
string(REPLACE "\n" ";" lines "${content}")

set(state "")          # what the quoted continuation lines belong to
set(msgid "")
set(msgctxt "")
set(entry_conflict FALSE)
set(conflicts "")

macro(close_entry)
    if(entry_conflict AND NOT msgid STREQUAL "")
        if(msgctxt STREQUAL "")
            list(APPEND conflicts "\"${msgid}\"")
        else()
            list(APPEND conflicts "\"${msgid}\" (context \"${msgctxt}\")")
        endif()
    endif()
    set(msgid "")
    set(msgctxt "")
    set(entry_conflict FALSE)
endmacro()

foreach(line IN LISTS lines)
    if(line MATCHES "^msgctxt \"(.*)\"$")
        close_entry()
        set(msgctxt "${CMAKE_MATCH_1}")
        set(state ctxt)
    elseif(line MATCHES "^msgid \"(.*)\"$")
        if(NOT state STREQUAL "ctxt")
            close_entry()
        endif()
        set(msgid "${CMAKE_MATCH_1}")
        set(state id)
    elseif(line MATCHES "^msgid_plural ")
        set(state plural)
    elseif(line MATCHES "^msgstr")
        set(state str)
        if(line MATCHES "#-#-#-#-#")
            set(entry_conflict TRUE)
        endif()
    elseif(line MATCHES "^\"(.*)\"$")
        set(text "${CMAKE_MATCH_1}")
        if(state STREQUAL "id")
            string(APPEND msgid "${text}")
        elseif(state STREQUAL "ctxt")
            string(APPEND msgctxt "${text}")
        elseif(state STREQUAL "str" AND text MATCHES "#-#-#-#-#")
            set(entry_conflict TRUE)
        endif()
    endif()
endforeach()
close_entry()

if(conflicts)
    list(JOIN conflicts "\n  " text)
    string(REPLACE "@SEMI@" ";" text "${text}")
    string(REPLACE "@LB@" "[" text "${text}")
    string(REPLACE "@RB@" "]" text "${text}")
    message(FATAL_ERROR
        "${DOMAIN} (${LANG}): translated differently in two fragments:\n  ${text}\n"
        "Keep one translation, in one fragment.")
endif()
