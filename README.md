# About

**libdeltacloud for CloudModule**

This is forked from libdeltacloud <https://git.fedorahosted.org/git/deltacloud/libdeltacloud.git>.

Original libdeltacloud is written by Mr.Chris Lalancette.
It is published under the LGPL license.

"libdeltacloud for CloudModule" is made for "CloudModule for Zabbix".

# Diff with original libdeltacloud

This customize module is added Metric monitoring features.

# Usage

Download source code.
And, compile & install.

    $ git clone https://github.com/ike-dai/libdeltacloud-for-cloudmodule.git
    $ cd libdeltacloud-for-cloudmodule
    $ ./configure
    $ make
    $ sudo make install

As default, this library will be installed in /usr/local/lib.
If you change the installation path, please set options.

    $ ./configure --prefix=<path>


