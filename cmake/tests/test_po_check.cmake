# The merged-catalog check on files shaped like msgcat's output.
#   cmake -DWORK=<dir> -DSCRIPT=<RayeCheckPo.cmake> -P test_po_check.cmake

file(MAKE_DIRECTORY ${WORK})

function(run name content want_ok want_text)
    file(WRITE ${WORK}/${name}.po "${content}")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -DPO_FILE=${WORK}/${name}.po -DDOMAIN=raye-test -DLANG=fr -P ${SCRIPT}
        RESULT_VARIABLE rc ERROR_VARIABLE err OUTPUT_QUIET)
    set(ok FALSE)
    if(rc EQUAL 0)
        set(ok TRUE)
    endif()
    if(NOT ok STREQUAL want_ok)
        message(SEND_ERROR "${name}: passed=${ok}, wanted ${want_ok}\n${err}")
        set(failed TRUE PARENT_SCOPE)
    elseif(want_text)
        string(FIND "${err}" "${want_text}" at)
        if(at EQUAL -1)
            message(SEND_ERROR "${name}: message does not name ${want_text}:\n${err}")
            set(failed TRUE PARENT_SCOPE)
        endif()
    endif()
endfunction()

set(header [=[
msgid ""
msgstr ""
"#-#-#-#-#  core.po  #-#-#-#-#\n"
"Content-Type: text/plain; charset=UTF-8\n"
"#-#-#-#-#  settings.po  #-#-#-#-#\n"
"Content-Type: text/plain; charset=UTF-8\n"
]=])

set(failed FALSE)

# headers always differ: not a conflict
run(header_only "${header}
msgid \"Close\"
msgstr \"Fermer\"
" TRUE "")

run(conflict "${header}
msgid \"Close\"
msgstr \"\"
\"#-#-#-#-#  core.po  #-#-#-#-#\\n\"
\"Fermer\\n\"
\"#-#-#-#-#  settings.po  #-#-#-#-#\\n\"
\"Quitter\"
" FALSE "\"Close\"")

# a message on several lines, with a semicolon and brackets in it
run(multiline "${header}
msgid \"\"
\"Restart now; \"
\"[later]\"
msgstr \"\"
\"#-#-#-#-#  core.po  #-#-#-#-#\\n\"
\"Redémarrer\\n\"
\"#-#-#-#-#  settings.po  #-#-#-#-#\\n\"
\"Plus tard\"
" FALSE "Restart now; [later]")

run(context "${header}
msgctxt \"menu\"
msgid \"Open\"
msgstr \"\"
\"#-#-#-#-#  core.po  #-#-#-#-#\\n\"
\"Ouvrir\\n\"
\"#-#-#-#-#  settings.po  #-#-#-#-#\\n\"
\"Ouvre\"
" FALSE "context \"menu\"")

run(plural "${header}
msgid \"%1 tab\"
msgid_plural \"%1 tabs\"
msgstr[0] \"%1 onglet\"
msgstr[1] \"%1 onglets\"

msgid \"Save\"
msgstr \"Enregistrer\"
" TRUE "")

if(failed)
    message(FATAL_ERROR "po check: some checks failed")
endif()
message(STATUS "po check: all checks passed")
