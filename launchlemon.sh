#!/bin/sh

handler() {
	while read line; do
		echo "Action requested: $line"
		$line &
	done
}

project_monitor() {
	while read line; do echo -e "%{S0}$line%{S1}$line"; done
}

start_top() {
	bin/topbar 2>/tmp/lemonbar1.log | lemonbar -p -g x24++ \
		-F#ffffff -B#222222 -U#268BD2 -u 2 \
		-f "FreeMono:size=10" | handler
}

start_bottom() {
	bin/bottombar 2>/tmp/lemonbar2.log | lemonbar -b -p -g x24++ \
		-F#ffffff -B#222222 -U#268BD2 -u 2 \
		-f "FreeMono:size=10" \
		-f "Font Awesome 6 Free"  \
		-f "Font Awesome 6 Brands" \
		-f "Font Awesome 6 Free Solid" | handler
}

kill() {
	pkill -f bin/topbar
	pkill -f bin/bottombar
	pkill -x lemonbar
	pkill -f lemonbar/scripts
}

kill
source /home/aarya/pyvenv/bin/activate
cd /home/aarya/repos/aarya-bhatia/lemonbar
make
start_top &
start_bottom &
echo "lemonbar launched..."

