# Pull base image
FROM ubuntu:16.04

LABEL maintainer="katarzyna.wasiuta@intel.com"

ENV TERM xterm \
    DEBIAN_FRONTEND noninteractive

# Update the Apt cache and install basic tools
RUN apt-get update && apt-get install -y \
    asciidoc \
    autoconf \
    build-essential \
    devscripts \
    g++ \
    gcc \
    git \
    libfuse-dev \
    libc6-dbg \
    libglib2.0-dev \
    libjson-c-dev \
    libkmod-dev \
    libncurses5-dev \
    libnuma-dev \
    libpthread-stubs0-dev \
    libtool \
    libudev-dev \
    libunwind8-dev \
    libuv1-dev \
    make \
    numactl \
    python-setuptools \
    python-dev \
    software-properties-common \
    sudo \
    uuid-dev \
    whois

# Add user
ENV USER memkinduser
ENV USERPASS memkindpass
RUN useradd -m $USER -g sudo -p `mkpasswd $USERPASS`
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

COPY docker.sh docker.sh
COPY codecov-script.sh /home/$USER/codecov-script.sh

RUN chown -R $USER:sudo /mnt
RUN chmod -R 777 /mnt
USER $USER
RUN git config --global user.email "memkinduser@intel.com"
RUN git config --global user.name "memkinduser"



