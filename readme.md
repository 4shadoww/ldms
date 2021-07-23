ldms
====
Ldms is an anti-forensics program to add extra security to prevent cold boot attacks and other attacks which could be performed when an attacker has physical access to the machine.

It doesn't give any kind of full immunity to your system from these attacks but still, it improves OpSec and some cases may disable an attack vector.

Ldms is not meant to run on every possible system, but on computers that are left alone while power is on and have encrypted partitions(for example servers).

Build and installion
====================
To build ldms you need to have a c++11 compliant compiler (GCC is recommended), GNU make and have installed all dependencies.

A basic GNU/Linux installation should include all dependencies except libsensors (lm_sensors) which you will have to install (including header files).

Example output:

    $ make -j 4
    g++ -Wall -DLDMS_DAEMON  -Iinclude -pthread -lsensors src/ldmsd.cpp  src/config_loader.cpp src/globals.cpp src/logging.cpp src/modules/usb_events.cpp src/modules/lm_sensors.cpp src/modules/network.cpp -o ldmsd
    g++ -Wall -Iinclude -pthread -lsensors src/ldms.cpp src/config_loader.cpp src/globals.cpp src/logging.cpp -o ldms

After building you should have two executables *ldms* and *ldmsd*.

You can install ldms by using the install make target:

    # make install
    install ldmsd /usr/bin/
    install ldms /usr/bin/
    install -m 644 ldmsd.service /usr/lib/systemd/system/
    install -m 644 ldmsd.timer /usr/lib/systemd/system/
    install -m 644 -D example.conf /var/lib/ldms/example.conf
    install -m 644 -D example.conf /etc/ldms/ldmsd.conf
    gzip man/ldms.1 -c > man/ldms.1.gz
    gzip man/ldmsd.1 -c > man/ldmsd.1.gz
    gzip man/ldmsd.conf.5 -c > man/ldmsd.conf.5.gz
    install -m 644 man/ldms.1.gz /usr/share/man/man1/ldms.1.gz
    install -m 644 man/ldmsd.1.gz /usr/share/man/man1/ldmsd.1.gz
    install -m 644 man/ldmsd.conf.5.gz /usr/share/man/man5/ldmsd.conf.5.gz
    
There is also uninstall target to uninstall the program. By default, config files are not deleted from */etc/ldms*.

**Important** be aware currently the install target will install systemd unit files. So if you are running some other init system, then obviously you don't want to install those.

Compiling without lm-sensors module
-----------------------------------

If you don't need or want the lm-sensors module, you can make a build without it by defining a "NO_LMSENSORS" macro:

    make debug -j 4 CFLAGS=-DNO_LMSENSORS
    g++ -DNO_LMSENSORS -DLDMS_DAEMON  -Iinclude -pthread -lsensors src/ldmsd.cpp  src/config_loader.cpp         src/globals.cpp src/logging.cpp src/modules/usb_events.cpp src/modules/lm_sensors.cpp src/modules/network.cpp src/modules/checkin.cpp -o ldmsd
    g++ -DNO_LMSENSORS -Iinclude -pthread -lsensors src/ldms.cpp src/config_loader.cpp src/globals.cpp  src/logging.cpp -o ldms

Configuration
=============
The configuration file is located at */etc/ldms/ldmsd.conf*. Config file has comments but they don't have explanations. It's recommended to read man page ldmsd.conf(5).

You should at least set a value to *command* and *modules* variables. Modules should work with default values in the configuration file, but still, you should check they are ok.

*command* variable defines the command ran upon triggering event. See the example shell script below which could be used. *modules* variable defines which modules will be used.

Basic usage
===========
Ldms consists of a daemon and a control program that controls when the daemon armed. You want to think very carefully when you want to arm ldms. Generally if you are tweaking something you want to disarm it. This depends on your configuration and use case.

Example of control program usage:

    # ldms status
    switch is disarmed
    # ldms arm
    ldms is now armed and command "echo "dead man's switch triggered"" will be ran upon triggering!!!
    make sure ldms daemon is running!!!
    # ldms status
    switch is armed
    # ldms disarm
    ldms is now disarmed!!!

You should start the daemon with systemd and not use screen or something else to run it.

Example of running with systemd:

    # systemctl start ldmsd
    
You can also enable the daemon, but you cannot simply enable *ldmsd.service*. You'll have to use a timer to start the daemon to ensure it doesn't start too early. You can do that with this command:

    # systemctl enable ldmsd.timer
    Created symlink /etc/systemd/system/timers.target.wants/ldmsd.timer → /usr/lib/systemd/system/ldmsd.timer.
    
The timer starts the daemon after one minute. Using a timer is **very important**. Without it, the daemon will start too early and may not let the system boot correctly if it's armed.

Example script
--------------
Here is an example shell script which could be used:
    
    #!/usr/sh
    
    # example.sh
    # Unmount the drive
    umount /mnt
    # Close drive
    cryptsetup close drive
    # Clear PageCache, dentries and inodes
    sync; echo 3 > /proc/sys/vm/drop_caches
    # And finally shut down
    shutdown now

You could also send a notification email before shutting down.

Bug reports
===========
Please leave bug reports only to https://gitlab.com/4shadoww/ldms.
