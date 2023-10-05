# Set up environment to compile Contiki-NG code:

### WSL2 for Windows 
https://ubuntu.com/tutorials/install-ubuntu-on-wsl2-on-windows-10#1-overview
Ubuntu 20.04 LTS Image

## Essential Linux commands (ignore the $ sign): 
Changing directory in terminal: 

	$ cd <relative path or absolute path> 
List all files and directories in current directory: 

	$ ls
removing a file: 
	
 	$ rm <file>
removing a directory:
	
 	$ rm -r <directory>


## Set up Linux toolchain: 
https://docs.contiki-ng.org/en/develop/doc/getting-started/Toolchain-installation-on-Linux.html?highlight=git%20submodule%20#install-development-tools-for-contiki-ng

First, complete the steps listed under "Install development tools for Contiki-NG" section.
Secondly, complete the steps to "Install ARM compiler", and then add it to your path environment variable (how-to https://askubuntu.com/a/60219).

#### Clone Contiki-NG-NES at a chosen directory
**FOR THIS PROJECT, KEEP IN MIND THAT YOU SHOULD CLONE THIS REPOSITORY (https://github.com/Project-Repositories/contiki-ng-NES), AS WE ARE MAKING THE MODIFICATIONS HERE. DO NOT CLONE THE OFFICIAL CONTIKI-NG REPOSITORY.**

A great speedup in compliation happens if the Github Repository is cloned to a subdirectory in your Ubuntu VM, compared to mounting the windows directory and accessing the files through WSL.

Switches between a >1 sec to 10> sec compilation of helloworld from testing

Remember to pull the submodules of this repository, as illustrated in the last step of "Clone Contiki-NG" in the Toolchain installation guide.

#### Compile Hello-World and run on native:
https://docs.contiki-ng.org/en/develop/doc/tutorials/Hello%2C-World%21.html

#### Compile Hello-World for CC2650
Be at the HelloWorld directory, then use the following command:

	$ make TARGET=cc26x0-cc13x0 BOARD=launchpad/cc2650

## Flashing LaunchPad:
Install Uniflash on your Windows host OS 
https://www.ti.com/tool/UNIFLASH

Connect LaunchPad to PC using USB Cable

Select the device from "Detected Device"

"Browse" -> go to Hello World directory in cloned Contiki-NG-NES repository, probably called 
    
    \\wsl.localhost\Ubuntu\home\<user>\<path to where you cloned contiki-ng-NES repository>\<hello_world> 

Make sure to display all file types as option in file viewer then choose 

"hello-world.cc26x0-cc13x0", and then click "Load image"

## Talk with LaunchPad:

Install PuTTy or similar serial program

In PuTTy, choose Serial option

Find the appropriate "Serial line" option by going to 

    Windows Device Manager -> Ports (COM & LPT) -> XDS110 Class Application/User UART

Then enter the COM# found above (for me it was, COM6).

Baudrate/speed = 115200

Now you should be able to see the Terminal printing "Hello, World" repeatedly
