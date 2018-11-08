#Pull base image
FROM ubuntu:16.04

ENV TERM xterm \
    DEBIAN_FRONTEND noninteractive

# Update the Apt cache and install basic tools
RUN apt-get update && apt-get install -y software-properties-common \
        libunwind8-dev autoconf python-setuptools python-dev \
        devscripts git gcc sudo libc6-dbg libncurses5-dev \
        libuv1-dev libfuse-dev libglib2.0-dev libtool whois \
        numactl libnuma-dev libpthread-stubs0-dev asciidoc \
        libjson-c-dev uuid-dev libkmod-dev libudev-dev \
        autoconf make build-essential g++

# Add user
ENV USER memkinduser
ENV USERPASS memkindpass
RUN useradd -m $USER -g sudo -p `mkpasswd $USERPASS`
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

COPY docker.sh docker.sh

RUN chown -R $USER:sudo /mnt
RUN chmod -R 777 /mnt
USER $USER
RUN git config --global user.email "memkinduser@intel.com"
RUN git config --global user.name "memkinduser"



