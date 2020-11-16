# **README**

This is utils/qemu/README.

Scripts in this directory are useful for HMAT emulation.

## Dependencies

- [QEMU](https://www.qemu.org/download/#source) (v5.0.0 or later)
- python3 and dependencies listed in [requirements.txt](https://github.com/memkind/memkind/blob/master/utils/qemu/requirements.txt)
- TODO: docker support, add instruction for topologies how to add new

## Create QEMU Image/Install OS
To set up your own guest OS image, please follow [instructions](https://en.wikibooks.org/wiki/QEMU/Images#Creating_an_image)

Example:
```bash
      qemu-img create -f qcow2 linux-ubuntu.img 30g
      wget https://releases.ubuntu.com/20.04.1/ubuntu-20.04.1-desktop-amd64.iso
      sudo qemu-system-x86_64 -cdrom ubuntu-20.04.1-desktop-amd64.iso -cpu host -enable-kvm -m 4G -vnc :5 -drive file=linux-ubuntu.img,format=qcow2 -boot d
```
- TODO check if server clouding [image](https://cloud-images.ubuntu.com/releases/focal/release-20201111/ubuntu-20.04-server-cloudimg-amd64.img) could be used

## Connection to QEMU

- with VNC (port 5905)

# Prerequisites
TODO
- automatically download OS and install (https://linux.die.net/man/1/virt-install + Kickstart file)
- setup PROXY (apt/dnf/git)
- install necessary packages: as memkind mandatory dependencies and hwloc, openssh-server
- setup GRUB parameters  in guest to no-graphics mode etc/default/GRUB: GRUB_CMDLINE_LINUX="console=tty0 console=ttyS0"
- setup GRUB parameters in guest to speed up boot etc/default/GRUB: GRUB_HIDDEN_TIMEOUT=0 GRUB_HIDDEN_TIMEOUT_QUIET=true
- provide ssh key exchange without user intervention - automate this
- standardized username in image - currently script below use "test" as user name

# SSH Key exchange Host->Guest
**HOST:** Generate SSH key.
```bash
    ssh-keygen -b 2048 -t rsa -q -N "" -f ./qemukey
```

**HOST:** Copy generated public key to guest.
```bash
    ssh-copy-id -p 10022 -i qemukey test_user_name@localhost
```
**GUEST:** Edit guest ssh configuration.
```bash
    vim /etc/ssh/sshd_config
```

```
    PasswordAuthentication no
    ChallengeResponseAuthentication no
```

**GUEST:** Reload configuration.
```bash
    sudo systemctl restart ssh.service
```

**HOST:** Verify SSH connection.
```bash
    ssh localhost -p 10022 ./qemukey
```

# For the development:

```bash
    sudo python3 main.py --image=qemu-linux.img --mode=dev --interactive
```
This will run the QEMU with specified image in bash shell in the QEMU.

```bash
    sudo python3 main.py --image=qemu-linux.img  --mode=dev --topology topology/<topology_file>
```
This will run QEMU image with specified memory topology defined in **topology_file**.

# For the tests:

```bash
    sudo python3 main.py --image=qemu-linux.img --mode=test --force_reinstall
```
This will the QEMU tests for all topologies placed in topology directory and rebuild and install memkind on QEMU image.

```bash
    sudo python3 main.py --image=qemu-linux.img  --mode=test --topology topology/<topology_file>
```
This will the QEMU test only for for specified memory topology defined in **topology_file**.

For other options please see:

```bash
    python3 main.py --help
```