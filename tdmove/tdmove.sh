#!/usr/bin/env bash

this=${0##*/}
this_b=${this%.*}
usage() {
    cat <<EOF
Usage: $0 <-s instance[.database.table]> <-t target instance.database.table> [-f select sql file] [-e fastexport|bteq] [-l fastload|bteq] [-m land file]
EOF
    exit 1
}

OPT=$(getopt -o s:t:f:e:l:m: -l log: -- $*)
[ $? -ne 0 ] && exit 1
eval set -- "$OPT"

while true;
do
    case "$1" in
        -s) tdsource=$2 ; shift 2 ;;
        -t) tdtarget=$2 ; shift 2 ;;
        -f) sql=$2 ; shift 2 ;;
        -e) exp=$2 ; shift 2 ;;
        -l) load=$2 ; shift 2 ;;
        -m) land=$2 ; shift 2 ;;
        --log) log=$2 ; shift 2 ;;
        --) shift ; break ;;
        *) echo "error!" ; exit 1 ;;
    esac
done

[ "$tdsource" == "" -o "$tdtarget" == "" ] && usage

instance_source=$(echo $tdsource | awk -F. '{print $1}')
db_source=$(echo $tdsource | awk -F. '{print $2}')
table_source=$(echo $tdsource | awk -F. '{print $3}') 

[ \( "$db_source" == "" -o "$table_source" == "" \) -a "$sql" == "" ] && usage

instance_target=$(echo $tdtarget | awk -F. '{print $1}')
db_target=$(echo $tdtarget | awk -F. '{print $2}')
table_target=$(echo $tdtarget | awk -F. '{print $3}')

[ "$db_target" == "" -o "$table_target" == "" ] && usage

[ "$sql" != "" ] && [ ! -f "$sql" ] && { echo "$sql not found." ; exit 1 ; }

exp=${exp:-fastexport}
load=${load:-fastload}

[ -z "$land" ] && { land=$(mktemp -d)/fifo ; mkfifo $land ; }
echo $land
ts=$(date '+%Y%m%d%H%M%S')
[ -z "$log" ] && log=$this_b.$ts.log
echo $log
# main
