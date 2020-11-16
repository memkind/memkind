# README

This is utils/qemu/README.

Scripts in this directory are useful for HMAT emulation

## Dependencies

- [Libvirt](https://libvirt.org/sources/) (v6.9.0 or later)
- [Packer](https://learn.hashicorp.com/tutorials/packer/getting-started-install?in=packer/getting-started) (v5.0 or later)
- [QEMU](https://www.qemu.org/download/#source) (v5.0.0 or later)
- python3.5 and dependencies listed in [requirements.txt](https://github.com/memkind/memkind/blob/master/utils/qemu/requirements.txt)
- TODO: docker support, add instruction for topologies how to add new

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
This will run the QEMU with specified image in bash shell in the QEMU.

```bash
    sudo python3 main.py --image=qemu-image/ubuntu-1804.img  --mode=dev --topology topology/<topology_file>
```
This will run QEMU image with specified memory topology defined in **topology_file**.

## For the tests:

```bash
    sudo python3 main.py --image=qemu-image/ubuntu-1804.img --mode=test --force_reinstall
```
This will the QEMU tests for all topologies placed in topology directory and rebuild and install memkind on QEMU image.

```bash
    sudo python3 main.py --image=qemu-image/ubuntu-1804.img  --mode=test --topology topology/<topology_file>
```
This will the QEMU test only for for specified memory topology defined in **topology_file**.

For other options please see:

```bash
    python3 main.py --help
```
