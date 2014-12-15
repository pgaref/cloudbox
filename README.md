cloudbox 
========
A Cloud tool for storing files in a distributed way!

![alt tag](https://raw.github.com/pgaref/cloubox/master/extras/Screenshot%202014-12-15%2001.03.07.png)

General Type Messages:
2 First Bytes | Type
0x01 | Status Message
0x02 | Client directory with no changes
0x03 | File Added
0x04 | File Modyfied
0x05 | File Deleted
0x06 | Client Requesting File 
0x07 | Client Offering a File
0x08 | Empty Directory
0xFFFF | No Action - For debugging!
