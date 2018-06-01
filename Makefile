all: compile_CS compile_WS compile_user

compile: compile_CS compile_WS compile_user

compile_and_execute: compile_CS compile_WS compile_user execute
	exit

execute: CS WS user

user: compile_user
	xterm -hold -title "user" -e ./user &

CS: compile_CS
	xterm -hold -title "CS" -e ./CS &

WS: compile_WS
	xterm -hold -title "WS" -e ./WS FLW UPP LOW WCT &

compile_user:
	gcc user.c -o user

compile_CS: direct
	gcc CS.c -o CS

compile_WS: direct
	gcc WS.c -o WS

direct:
	mkdir -p input_files output_files

clear:
	rm -rf user CS WS input_files output_files
