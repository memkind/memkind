# README

This is utils/qemu/README.

Scripts in this directory are useful for HMAT emulation

## Dependencies (the order matters)

- [QEMU](https://www.qemu.org/download/#source) (v5.1.0 or later)
- [Libvirt](https://libvirt.org/sources/) (v6.10.0 or later)
- Restart libvritd service after the installation
- [Packer](https://learn.hashicorp.com/tutorials/packer/getting-started-install?in=packer/getting-started) (v1.6.5 or later)
- TODO: add instruction for topologies how to add new

## Python Dependencies
- Python3.5
- fabric, psutil

## Files

* 'ubuntu-packer.json' is used to build the Ubuntu Server 18.04 image with Packer
* 'http/preseed.cfg' is used to provide the variables required for the quiet installation of the Ubuntu Server 18.04
* 'main.py' is a wrapper on QEMU to provide convenient way to handle different memory architectures.

## Building the QEMU image

In order to run the Ubuntu Server 18.04 installation run the following command:

```bash
    sudo PACKER_LOG=1 packer build ubuntu-packer.json
```

## For the development process:

```bash
    sudo python3 main.py --image=qemu-image/ubuntu-1804.img --mode=dev --interactive
```
This will run specified QEMU image in a bash shell.

```bash
    sudo python3 main.py --image=qemu-image/ubuntu-1804.img  --mode=dev --topology topology/<topology_file>
```
This will run specified QEMU image with memory topology defined in **topology_file**.

## For the tests:

```bash
    sudo python3 main.py --image=qemu-image/ubuntu-1804.img --mode=test --force_reinstall
```
This will run the QEMU tests for all topologies placed in topology directory and rebuild and reinstall memkind on the QEMU image.

```bash
    sudo python3 main.py --image=qemu-image/ubuntu-1804.img  --mode=test --topology topology/<topology_file>
```
This will run the QEMU tests only with memory topology defined in **topology_file**.

For other options, please see:

```bash
    python3 main.py --help
```
