# RCOM Project1

## Project Structure

- bin/: Compiled binaries.
- src/: Source code for the implementation of the link-layer and application layer protocols.
- include/: Header files of the link-layer and application layer protocols.
- cable/: Virtual cable program to help test the serial port.
- main.c: Main file.
- Makefile: Makefile to build the project and run the application.
- penguin.gif: Example file to be sent through the serial port.

## Instructions to Run the Project

1. Compile the application and the virtual cable program using the provided Makefile.
2. Run the virtual cable program (either by running the executable manually or using the Makefile target):

	```bash
	$ ./bin/cable_app
	$ make run_cable
	```

3. Test the protocol without cable disconnections and noise

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

4. Test the protocol with cable disconnections and noise  
	4.1. Run receiver and transmitter again  
	4.2. Quickly move to the cable program console and press 0 for unplugging the cable, 2 to add noise, and 1 to normal  
	4.3. Check if the file received matches the file sent, even with cable disconnections or with noise
