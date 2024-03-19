#!/bin/sh

handler() {
	while read line; do
		$line &
	done
}

project_monitor() {
	while read line; do echo -e "%{S0}$line%{S1}$line"; done
}

start_top() {
	bin/main config_top | project_monitor |
		lemonbar -p -g x24++ \
		-F#ffffff -B#222222 -U#268BD2 -u 2 \
		-f "FreeMono:size=10" \
		-f "Font Awesome 6 Free"  \
		-f "Font Awesome 6 Brands" \
		-f "Font Awesome 6 Free Solid" | handler
}

start_bottom() {
	bin/main config_bottom | project_monitor |
		lemonbar -b -p -g x24++ \
		-F#ffffff -B#222222 -U#268BD2 -u 2 \
		-f "FreeMono:size=10" \
		-f "Font Awesome 6 Free"  \
		-f "Font Awesome 6 Brands" \
		-f "Font Awesome 6 Free Solid" | handler
}

kill() {
	pkill -f /home/aarya/.config/lemonbar/bin/main
	pkill -f /home/aarya/scripts/lemonbar
	pkill -x lemonbar
}

kill
source /home/aarya/pyvenv/bin/activate
cd /home/aarya/repos/aarya-bhatia/lemonbar
make
start_top  &
start_bottom  &
echo "lemonbar launched..."

