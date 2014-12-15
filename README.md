cloudbox 
========
A Cloud tool for storing files in a distributed way!

![alt tag](https://github.com/pgaref/cloudbox/blob/master/extras/Screenshot%202014-12-15%2001.03.07.png)

Each Cloudbox client is able to recognize changes in a directory of files that monitors and notify all other clients running on the same subnet, sending a broadcast message using UDP. Each client receiving a broadcast message, checks the state of its directory and if  an update is needed  it connects to the appropriate client using TCP, to start the file transfer.

For the clients communication we currently support these general types of messages:

| 2 First Bytes | Type |
|-------------- | ---- |
0x01 | Status Message
0x02 | Client directory with no changes
0x03 | File Added
0x04 | File Modyfied
0x05 | File Deleted
0x06 | Client Requesting File 
0x07 | Client Offering a File
0x08 | Empty Directory
0xFFFF | No Action - For debugging!

TODO.. Add more Implementation details.

Currently supporting: Multiple threads,  locking files using flock(), client statistics (number of broadcast messages sent, KiloBytes transfered, average tranfer speed)
