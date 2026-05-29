# EnergyAwareIoT
There are three modules, 
1. Theoretical (Mathematical model of RL)

* one for the theoretical (Mathematical model) evaluation, which involves the reinforcement learning-based Q learning computation
* The higher the value, the better the state transition, which is programmed in C language for various state transitions
* For this we have used a tuple like <H/L, P, R, T>  

2. Simulation with NS3
   OS USed: Ubuntu 24.04
   ns3 version: ns-3.44
   The following link is used for installing ns-3.44 (https://www.youtube.com/watch?v=XgMeAV5shFg)
   Step-by-Step Guide to Install NS-3.44 on Ubuntu 24.04
Follow these steps carefully to set up NS-3.44 on your system.
Step 1: Update Your System
First, ensure your Ubuntu 24.04 system is up to date. Open a terminal (Ctrl + Alt + T) and run:
$ sudo apt update && sudo apt upgrade -y

Step 2: Install Required Dependencies
NS-3.44 requires several development tools and libraries. Install them with the following command:

$ sudo apt install g++ python3 cmake ninja-build git gir1.2-goocanvas-2.0 python3-gi python3-gi-cairo python3-pygraphviz gir1.2-gtk-3.0 ipython3 tcpdump wireshark sqlite3 libsqlite3-dev qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools openmpi-bin openmpi-common openmpi-doc libopenmpi-dev doxygen graphviz imagemagick python3-sphinx dia imagemagick texlive dvipng latexmk texlive-extra-utils texlive-latex-extra texlive-font-utils libeigen3-dev gsl-bin libgsl-dev libgslcblas0 libxml2 libxml2-dev libgtk-3-dev lxc-utils lxc-templates vtun uml-utilities ebtables bridge-utils libxml2 libxml2-dev libboost-all-dev ccache python3-full python3-pip

Step 3: Download NS-3.44 Source Code
Next, download the NS-3.44 source code from the official website or Git repository. For simplicity, we’ll use the tarball method.

$ wget https://www.nsnam.org/releases/ns-allinone-3.44.tar.bz2
$ tar -xjf ns-allinone-3.44.tar.bz2
$ cd ns-allinone-3.44
$ ./build.py --enable-examples --enable-tests

After the installation, the two files implemented ee-iot-wifi.cc and ee-iot-wpan.cc were copied to ~ns-3.44/scratch folder and run using the following command 

$ ./ns3 run scratch/ee-iot-wifi.cc to get the performance metrics when used as a Wi-Fi with the IEEE 802.11 standard

$ ./ns3 run scratch/ee-iot-wpan.cc to get the performance metrics when used as a Wireless Personal Area Network (IEEE 802.15.4- IOT) with IEEE 802.15.4 standard

3. Experimentation with MicaZ motes (MTS310 motes)
   A dataset is also collected based on sensors, namely temperature, accelerometer, and magnetometer.
   The dataset is included within this folder called dataset_raw.csv, and there are two images that show the creation of the dataset using the sensors MTS310.
   

   




