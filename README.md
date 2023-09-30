# Set up environment to compile Contiki-NG code:

### WSL2 for Windows 
https://ubuntu.com/tutorials/install-ubuntu-on-wsl2-on-windows-10#1-overview
Ubuntu 20.04 LTS Image

## Set up Linux toolchain: 
https://docs.contiki-ng.org/en/develop/doc/getting-started/Toolchain-installation-on-Linux.html?highlight=git%20submodule%20#install-development-tools-for-contiki-ng

Also install the ARM Compiler by add to the working-directory
    
    gcc-arm-none-eabi-9-2020-q2-update/bin to your path.

#### Clone Contiki-NG at a chosen directory
**FOR THIS PROJECT, YOU SHOULD CLONE THIS REPOSITORY (https://github.com/Project-Repositories/contiki-ng-NES), AS WE ARE MAKING THE MODIFICATIONS HERE. DO NOT CLONE THE OFFICIAL CONTIKI-NG REPOSITORY.**

A great speedup in compliation happens if the Github Repository is cloned to a subdirectory in your Ubuntu VM, compared to mounting the windows directory and running wsl through that

Switches between a >1 sec to 10> sec compilation of helloworld from testing

https://docs.contiki-ng.org/en/develop/doc/getting-started/Toolchain-installation-on-Linux.html?highlight=git%20submodule%20#clone-contiki-ng

Remember to get the submodules!

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

"Browse" -> go to Hello World directory in your Linux image probably called 
    
    \\wsl.localhost\Ubuntu\home\<user>\<path to where you cloned contiki-ng repository>\<hello_world> 

Make sure to display all file types as option in file viewer then choose 

"hello-world.cc26x0-cc13x0" "Load image"

## Talk with LaunchPad:

Install PuTTy or similar serial program

In PuTTy, choose Serial option

Find the appropriate "Serial line" option by going to 

    Windows Device Manager -> Ports (COM & LPT) -> XDS110 Class Application/User UART

Then enter the COM# found above (for me it was, COM6).

Baudrate/speed = 115200

Now you should be able to see the Terminal printing "Hello, World"



	
