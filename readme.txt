The project_server and project_client are linux c programs running on the board.
sendsignal is the c program running on windows.
gui.m is the matlab interface.
%%%%%%%%%%%%project_server%%%%%%%%%%%%%
1Run the faircom database on the board.
2.compiler:

activate the -static switch.

----- Compiler -----
Include paths (-I):
BASE_DIR_PATH/linux.v2.4.arm.32bit/include/
BASE_DIR_PATH/linux.v2.4.arm.32bit/include/sdk/ctree.ctdb/multithreaded/static/

----- Linker -----
Libraries (-l):  
mtclient         (this one has to come first)
pthread
dl
m

Library search path (-L):
BASE_DIR_PATH/linux.v2.4.arm.32bit/lib/ctree.ctdb/multithreaded/static/

3.the first time running. you need to comment the "start server"
	and uncomment the "manage", and uncomment the"addinfo" to add info table.
4run the program with the port number

%%%%%%%%%%%%project_client%%%%%%%%%%%%%
1. It should on the different board with the server.
2. run with the paramaters:
 hostname port usrno signal type(0:real, 1,2,3: simulated)
the history eeg file should name: "eeg1.txt","eeg2.txt"......
%%%%%%%%%%%%sendsignal%%%%%%%%%%%%%
 
This is the program send the signal data from the real eeg band.
Or you also can create a file name "EEG.txt" has the fake data.

1. compiler with the command -lws2_32 when using gcc.

%%%%%%%%%%%%gui%%%%%%%%%%%%%
1. should open in the matlab.
2. input the port, ip, userno and sequence
3. choose the time and range
4.  wait and then see the results.