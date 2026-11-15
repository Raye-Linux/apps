# Read by the session at login: the renamed .desktop files come before the
# stock ones. Sourced, so no exit and nothing printed.
raye_overrides=/usr/share/raye/app-overrides
case ":${XDG_DATA_DIRS:=/usr/local/share:/usr/share}:" in
    *":$raye_overrides:"*) ;;
    *) XDG_DATA_DIRS="$raye_overrides:$XDG_DATA_DIRS" ;;
esac
export XDG_DATA_DIRS
unset raye_overrides
