#! /bin/bash

logfile="/home/pi/programs/TAD/checkLog.txt"

case "$(pidof tad | wc -w)" in

0)  echo "tad not running, restarting tad:     $(date)" >> $logfile
    sudo /home/pi/programs/TAD/execTAD.sh &
    ;;
1)  echo "tad running, all OK:     $(date)" >> $logfile
    ;;
*)  echo "multiple instances of tad running. Stopping & restarting tad:     $(date)" >> $logfile
    kill -9 $(pidof tad | awk '{print $1}')
    export defunct=$(ps aux | grep -w Z)
    echo ' processo defunto: $defunct'
    ;;
esac
