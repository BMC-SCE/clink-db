#!/bin/bash

DB=xNULLx
SNAME=xNULLx
CXTYPES=xNULLx

procname=$(basename $0)
commands_needed="clink-db sqlite3"

function red ()
{
echo -e "\033[1;31m$1\033[0;39m"
}

silent()
{
    "$@" > /dev/null 2>&1
}

Usage ()
{
cat << EOH
Usage:
        $procname -f DB (clink-db.sqlite3) -n name (optional, symbol name) -t cx_category (optional, one or more numbers comma separated whithout spaces, run "clink-db -x" for all CXCursorKind)
EOH
exit 0
}

check_enviroment()
{
for file in $commands_needed; do
if ! (silent command -v $file); then red "$procname require $file to run" ; exitus 1; fi
done
}

search_db ()
{
if [ "$SNAME" == "xNULLx" ] && [ "$CXTYPES" == "xNULLx" ]; then
echo Execute SQL query: sqlite3 -header $DB "SELECT * FROM symbols ORDER BY name;"
sqlite3 -header $DB "SELECT * FROM symbols ORDER BY name;"
return $?
fi

if [ "$SNAME" != "xNULLx" ] && [ "$CXTYPES" == "xNULLx" ]; then
echo Execute SQL query: sqlite3 -header $DB "SELECT * FROM symbols WHERE name = '$SNAME' ORDER BY name;"
sqlite3 -header $DB "SELECT * FROM symbols WHERE name = '$SNAME' ORDER BY name;"
return $?
fi

if [ "$SNAME" == "xNULLx" ] && [ "$CXTYPES" != "xNULLx" ]; then
echo Execute SQL query: sqlite3 -header $DB "SELECT * FROM symbols WHERE cx_category IN ( $CXTYPES ) ORDER BY name;"
sqlite3 -header $DB "SELECT * FROM symbols WHERE cx_category IN ( $CXTYPES ) ORDER BY name;"
return $?
fi

if [ "$SNAME" != "xNULLx" ] && [ "$CXTYPES" != "xNULLx" ]; then
echo Execute SQL query: sqlite3 -header $DB "SELECT * FROM symbols WHERE name = '$SNAME' AND cx_category IN ( $CXTYPES ) ORDER BY name;"
sqlite3 -header $DB "SELECT * FROM symbols WHERE name = '$SNAME' AND cx_category IN ( $CXTYPES ) ORDER BY name;"
return $?
fi
}

if [ "$#" -lt 2 ]; then Usage; fi

while getopts ":f:n:t:" opt; do
 case "${opt}" in
     f)
       DB=$OPTARG
       if ! [ -a "$DB" ] ; then echo "$DB not found"; Usage; fi
     ;;
     n)
       SNAME=$OPTARG;
     ;;
     t)
       CXTYPES=$OPTARG
     ;;
     *)
       Usage
     ;;
  esac
done

shift $((OPTIND -1))

check_enviroment
search_db
