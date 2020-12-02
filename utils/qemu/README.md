# README

This is utils/qemu/README.

Scripts in this directory let you run the installation of the Ubuntu Server 18.04 image in QEMU environment.

## Files

* 'ubuntu-packer.json' is used to build the Ubuntu Server 18.04 image with Packer
* 'http/preseed.cfg' is used to provide the variables required for the quiet installation of the Ubuntu Server 18.04

## Prerequisites
At least Packer version 5.0 is required. More information on installing Packer [here](https://learn.hashicorp.com/tutorials/packer/getting-started-install?in=packer/getting-started)

## Building and running the image

In order to run the OS installation run the following command from the packer-scripts directory:
```$ sudo PACKER_LOG=1 packer build ubuntu-packer.json```
