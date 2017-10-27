killall -9 monitor_agent
./monitor_agent monitor.conf >>monitor.log
sleep 1

check=`ps -ef | grep monitor_agent | grep -v grep | wc -l`
if [ $check -eq 0 ]; then
        echo "Monitor agent fails to start, see monitor.log (below) for more details:"
        tail monitor.log
else
        echo "Monitor agent started"
fi

if (( $(crontab -l | grep "sysmon.py" | wc -l) == 0 )); then
    crontab -l > /tmp/crontab.sysmon
    echo '' >> /tmp/crontab.sysmon
    echo "## added system monitor report"  >> /tmp/crontab.sysmon
    echo "* * * * * /ecim/agent/monitor/sysmon.py >>/ecim/agent/monitor/sysmon.log 2>&1 &" >>/tmp/crontab.sysmon
    crontab /tmp/crontab.sysmon
    crontab -l
fi
