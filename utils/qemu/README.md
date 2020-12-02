# README

This is utils/qemu/README.

Scripts in this directory let you run the installation of the Ubuntu OS image in QEMU environment.

## Files

'ubuntu-packer.json' is used to build the OS image with Packer  
'http/preseed.cfg' is used to provide the variables required for the quiet installation of the Ubuntu OS

## Building and running the image

In order to run the OS installation run the following command from the packer-scripts directory:  
```$ sudo PACKER_LOG=1 packer build ubuntu-packer.json```