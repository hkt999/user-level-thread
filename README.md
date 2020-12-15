# user-level-thread
The exercise for the user level thread programming on Ubuntu 20.04, but other version Linux / UNIX can also work. Note: the ucontext API is deprecated after MacOS 10.6, add -D_XOPEN_SOURCE in gcc flags can make it work (tested on MacOS 10.15.6)

Build:
* Get the source code from github ("git clone https://github.com/hkt999/user-level-thread.git")
* Build the test program ("make")
* Run the test program ("./test")
