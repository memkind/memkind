# SPDX-License-Identifier: BSD-2-Clause
# Copyright (C) 2020 Intel Corporation.

import argparse
import collections
import fabric
import functools
import os
import pathlib
import psutil
import re
import subprocess
import sys
import time
import typing

TCP_PORT = 10022
VNC_DEFAULT_PORT = 5900
VNC_PORT = 5
GUEST_USERNAME = 'memkinduser'
GUEST_PASS = 'memkindpass'
MEMKIND_GUEST_PATH = '/mnt/memkind/'
MEMKIND_MOUNT_TAG = 'memkind_host'
MEMKIND_QEMU_PREFIX = 'utils/qemu'
TOPOLOGY_ENV_VAR = 'MEMKIND_TEST_TOPOLOGY'
TOPOLOGY_DIR = 'topology'
# TODO fix interactive option
# TODO update ccache links in original image
# TODO handling fsdax??
# TODO handling specific guest CPU - KNL ?
# TODO handling codecov in this script
# TODO handling scaling the memory in xml
# TODO provide also build with disabled hwloc
# TODO handling verbosity of this script

QemuCfg = collections.namedtuple('QemuCfg', ['workdir', 'image', 'interactive', 'force_reinstall', 'run_test', 'topologies'])
TopologyCfg = collections.namedtuple('TopologyCfg', ['name', 'hmat', 'cpu_options', 'mem_options'])


def _logger(func):
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        print(f'Start {func.__name__}')
        result = func(*args, **kwargs)
        print(f'Exit {func.__name__}')
        return result
    return wrapper


class MemoryHostException(Exception):
    """
    Exception raised for errors in case of insufficient memory on host.
    """

    def __init__(self, total_memory: int, tpg_memory: int, tpg_name: str) -> None:
        self.total_memory = total_memory
        self.tpg_memory = tpg_memory
        self.tpg = tpg_name

    def __str__(self) -> str:
        return (f'Memory on host) {self.total_memory} bytes are not enough '
                f'for memory required by {self.tpg} : {self.tpg_memory} bytes.')


class GuestConnection:

    CONNECT_TIMEOUT = 10

    def __init__(self, cfg: QemuCfg, topology_name: str) -> None:
        self.run_test = cfg.run_test
        self.force_reinstall = cfg.force_reinstall
        self.topology_name = topology_name

    @property
    def _connection_params(self) -> dict:
        """
        Connection parameters Host->Guest
        """
        return {'host': 'localhost',
                'user': f'{GUEST_USERNAME}',
                'connect_timeout': f'{self.CONNECT_TIMEOUT}',
                'connect_kwargs': {"password": f'{GUEST_PASS}'},
                'port': f'{TCP_PORT}'
                }

    @_logger
    def _build_and_reinstall_memkind(self, c: fabric.Connection) -> None:
        """
        Rebuild and reinstall memkind library
        """
        result = c.run('nproc')
        nproc = int(result.stdout.strip())
        c.run('make distclean', echo=True, warn=True)
        c.run('./autogen.sh', echo=True)
        c.run('./configure --prefix=/usr', echo=True)
        c.run(f'make -j {nproc}', echo=True)
        c.run(f'make checkprogs -j {nproc}', echo=True)
        c.run('sudo make uninstall', echo=True)
        c.run('sudo make install', echo=True)
        c.run('sudo ldconfig', echo=True)

    @_logger
    def _mount_workdir(self, c: fabric.Connection) -> None:
        """
        Mount working directory from host to guest
        """
        c.run(f'sudo mkdir -p {MEMKIND_GUEST_PATH}', echo=True)
        c.run(f'sudo mount {MEMKIND_MOUNT_TAG} {MEMKIND_GUEST_PATH} -t 9p -o trans=virtio', echo=True)

    def _set_test_env_name(self, c: fabric.Connection):
        """
        Set environment variable for topology
        """
        if self.topology_name:
            c.config.run.env = {f'{TOPOLOGY_ENV_VAR}': f'{self.topology_name}'}

    @_logger
    def _run_memkind_test(self, c: fabric.Connection) -> None:
        """
        Run memkind test
        """
        c.run('make unit_tests_hmat', echo=True)

    @_logger
    def _shutdown(self, c: fabric.Connection) -> None:
        """
        Shutdown guest
        """
        c.run('sudo shutdown now', echo=True, warn=True)

    @_logger
    def run_connection_test(self, force_reinstall: bool, qemu_pid: int) -> None:
        """
        Run QEMU in test mode
        """
        with fabric.Connection(**self._connection_params, inline_ssh_env=True) as c:
            try:
                self._mount_workdir(c)
                with c.cd(MEMKIND_GUEST_PATH):
                    self._set_test_env_name(c)
                    if force_reinstall:
                        self._build_and_reinstall_memkind(c)
                    if self.run_test:
                        self._run_memkind_test(c)
            finally:
                self._shutdown(c)
                psutil.wait_procs([psutil.Process(qemu_pid)], timeout=10)

    @_logger
    def run_connection_dev(self, force_reinstall: bool) -> None:
        """
        Run QEMU in dev mode
        """
        with fabric.Connection(**self._connection_params, inline_ssh_env=True) as c:
            try:
                self._mount_workdir(c)
                with c.cd(MEMKIND_GUEST_PATH):
                    self._set_test_env_name(c)
                    if force_reinstall:
                        self._build_and_reinstall_memkind(c)
                print(f'QEMU development is ready to use via SSH ({TCP_PORT}) or VNC ({VNC_DEFAULT_PORT + VNC_PORT})')
            except fabric.exceptions.GroupException:
                self._shutdown(c)


class QEMU:

    def __init__(self, cfg: QemuCfg) -> None:
        self.cfg = cfg
        self.pid = None

    @property
    def _qemu_exec(self) -> str:
        """
        QEMU binary
        """
        return 'qemu-system-x86_64'

    @property
    def _hda_option(self) -> str:
        """
        Hard drive as a file
        """
        return f'-hda {self.cfg.image}'

    @property
    def _optim_option(self) -> str:
        """
        Optimization options:
        - Use kernel based virtual machine
        """
        return '-enable-kvm -cpu host'

    @property
    def _connect_option(self) -> str:
        """
        Connect options:
        - VNC on port 5
        - tcp on port 10022
        """
        return (f'-vnc :{VNC_PORT} -netdev user,id=net0,hostfwd=tcp::{TCP_PORT}-:22 '
                f'-device virtio-net,netdev=net0')

    @property
    def _boot_option(self) -> str:
        """
        Boot order
        - Start from first virtual hard drive
        """
        return '-boot c'

    def _cpu_options(self, tpg: TopologyCfg) -> str:
        """
        CPU used by QEMU
        """
        return tpg.cpu_options if tpg.cpu_options else '-smp 2'

    def _memory_option(self, tpg: TopologyCfg) -> str:
        """
        Memory used by QEMU
        """
        if tpg.mem_options:
            mem_needed = 0
            all_sizes = re.findall(r'size=(\d+)', tpg.mem_options)
            for single_size in all_sizes:
                mem_needed += int(single_size)

            mem = psutil.virtual_memory()
            if mem_needed >= mem.total:
                raise MemoryHostException(mem.total, mem_needed, tpg.name)
            return f'-m {mem_needed/1024/1024}M'
        else:
            return '-m 2G'

    @property
    def _daemonize_option(self) -> str:
        """
        Daemonize option
        - run process as daemon or in terminal
        """
        return '-nographic' if self.cfg.interactive else '-daemonize'

    def _hmat_options(self, tpg: TopologyCfg) -> str:
        """
        HMAT option
        """
        return '-machine hmat=on' if tpg.hmat else ''

    def _memory_topology(self, tpg: TopologyCfg) -> str:
        """
        Memory topology string
        """
        return tpg.mem_options

    @property
    def _mount_option(self) -> str:
        """
        mount option
        - Start from first virtual hard drive
        """
        return (f'-fsdev local,security_model=passthrough,id=fsdev0,path={self.cfg.workdir} '
                f'-device virtio-9p-pci,id=fs0,fsdev=fsdev0,mount_tag={MEMKIND_MOUNT_TAG}')

    def _qemu_cmd(self, tpg: TopologyCfg) -> typing.List[str]:
        """
        QEMU command concat
        """
        cmd_str = ' '.join([self._qemu_exec,
                            self._hda_option,
                            self._optim_option,
                            self._connect_option,
                            self._boot_option,
                            self._memory_option(tpg),
                            self._memory_topology(tpg),
                            self._cpu_options(tpg),
                            self._hmat_options(tpg),
                            self._mount_option,
                            self._daemonize_option])
        print(f'QEMU command: {cmd_str}')
        return cmd_str.split()

    def _start_qemu_proc(self, qemu_cmd: typing.List[str]) -> None:
        """
        Start QEMU process
        """
        try:
            result = subprocess.run(qemu_cmd, stderr=subprocess.PIPE)
            result.check_returncode()
            for proc in psutil.process_iter():
                if proc.name() == self._qemu_exec:
                    self.pid = proc.pid
        except subprocess.CalledProcessError:
            sys.exit(f'\nQEMU cannot start process: image {self.cfg.image}: {result.stderr}.')

    def run(self) -> None:
        """
        run QEMU
        """
        reinstall_opt = self.cfg.force_reinstall
        for topology in self.cfg.topologies:
            try:
                qemu_cmd = self._qemu_cmd(topology)
            except MemoryHostException as e:
                print(f' QEMU ERROR: Memory topology skipped {topology.name}')
                print(f'{e}')
                continue
            if self.cfg.interactive:
                for sec in reversed(range(5)):
                    print(f'QEMU will start in {sec} seconds.')
                    time.sleep(1)
                self._start_qemu_proc(qemu_cmd)
            else:
                self._start_qemu_proc(qemu_cmd)
                time.sleep(10)
                con = GuestConnection(self.cfg, topology.name)
                if self.cfg.run_test:
                    con.run_connection_test(reinstall_opt, self.pid)
                else:
                    con.run_connection_dev(reinstall_opt)
                reinstall_opt = False


def find_root_memkind_dir() -> str:
    """
    Find memkind root directory on host
    """
    try:
        result = subprocess.run(['git', 'rev-parse', '--show-toplevel'], stdout=subprocess.PIPE)
        result.check_returncode()
        path = result.stdout.decode('utf-8').strip()
    except subprocess.CalledProcessError:
        # Get root directory without git modify this if we change the path of script
        path = str(pathlib.Path(__file__).parents[2])
    return path


def parse_topology_xml(tpg_file_name: str) -> TopologyCfg:
    """
    Parse topology xml file
    """
    try:
        result = subprocess.run(['virsh', 'domxml-to-native', 'qemu-argv', tpg_file_name], stdout=subprocess.PIPE)
        result.check_returncode()
        libvirt_args = result.stdout.decode('utf-8').strip()
        tpg_cfg = {'name': re.search(r'guest=(\w+)', libvirt_args).group(1),
                   'hmat': 'hmat=on' in libvirt_args,
                   'cpu_options': re.search('(?=-smp)(.*)threads=[0-9]+', libvirt_args).group(0),
                   'mem_options': re.search('(?=-object memory)(.*)(?=-uuid)', libvirt_args).group(1)}

        tpg = TopologyCfg(**tpg_cfg)
    except FileNotFoundError:
        sys.exit(f'\n XML file: {tpg_file_name} is missing.')
    except subprocess.CalledProcessError:
        sys.exit(f'\n XML file: {tpg_file_name} error in virsh parsing')
    return tpg


def parse_topology(cfg: dict) -> typing.List[TopologyCfg]:
    """
    Parse memory topology XML file
    """
    tpg_list = []
    single_topology = cfg.get('topology', None)
    if single_topology:
        tpg_list.append(parse_topology_xml(single_topology))
    elif cfg['mode'] == 'test':
        topology_dir = pathlib.Path(find_root_memkind_dir(), MEMKIND_QEMU_PREFIX, TOPOLOGY_DIR)
        for filepath in pathlib.Path.iterdir(topology_dir):
            if filepath.suffix == '.xml':
                tpg_list.append(parse_topology_xml(str(filepath)))
    else:
        dev_topology = {'name': '', 'hmat': False, 'cpu_options': '', 'mem_options': ''}
        tpg_list.append(TopologyCfg(**dev_topology))
    return tpg_list


def parse_arguments() -> QemuCfg:
    """
    Parse command line arguments
    """
    qemu_cfg = {'workdir': find_root_memkind_dir()}
    parser = argparse.ArgumentParser()
    parser.add_argument('--image', help='QEMU image', required=True)
    parser.add_argument('--mode', choices=['dev', 'test'], help='execution mode', default='dev')
    parser.add_argument('--topology', help='memory topology XML file')
    parser.add_argument('--interactive', action="store_true", help='execute an interactive bash shell', default=False)
    parser.add_argument('--force_reinstall', action="store_true", help='force rebuild and install memkind', default=False)
    cli_cfg = vars(parser.parse_args())

    if cli_cfg['interactive'] and cli_cfg['mode'] == 'test':
        parser.error("interactive is supported only by dev mode")

    qemu_cfg['image'] = cli_cfg['image']
    qemu_cfg['interactive'] = cli_cfg['interactive']
    qemu_cfg['force_reinstall'] = cli_cfg['force_reinstall']
    qemu_cfg['topologies'] = parse_topology(cli_cfg)
    qemu_cfg['run_test'] = cli_cfg['mode'] == 'test'
    return QemuCfg(**qemu_cfg)


@_logger
def main() -> None:
    """
    Main function
    """
    qemu_cfg = parse_arguments()
    qemu = QEMU(qemu_cfg)
    qemu.run()


if __name__ == "__main__":
    main()
