# RCOM Project 1

## Project Structure

- bin/: Compiled binaries.
- src/: Source code for the implementation of the link-layer and application layer protocols.
- include/: Header files of the link-layer and application layer protocols.
- main.c: Main file.
- Makefile: Makefile to build the project and run the application.
- penguin.gif: Example file to be sent through the serial port.

## Instructions to Run the Project

1. Compile the application using the provided Makefile.

	```bash
	$ make
	```

2. Run the socat command to create a virtual serial port pair.

	```bash
	$ sudo socat -d  -d  PTY,link=/dev/ttyS10,mode=777   PTY,link=/dev/ttyS11,mode=777
	```

3. Test the protocol by sending a file through the virtual serial port pair.

	3.1 Run the receiver (either by running the executable manually or using the Makefile target):

	```bash
	$ ./bin/main /dev/ttyS11 rx penguin-received.gif
	$ make run_tx
	```

	3.2 Run the transmitter (either by running the executable manually or using the Makefile target):

	```bash
	$ ./bin/main /dev/ttyS10 tx penguin.gif
	$ make run_rx
	```

	3.3 Check if the file received matches the file sent, using the diff Linux command or using the Makefile target:

	```bash
	$ diff -s penguin.gif penguin-received.gif
	$ make check_files
	```
