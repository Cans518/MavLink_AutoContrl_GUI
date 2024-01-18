#!/bin/bash

# Check if running with sudo
if [ "$EUID" -ne 0 ]; then
  echo "Please run with sudo or as root."
  exit
fi

# Update and upgrade
apt-get update
apt-get upgrade -y

# Install packages
apt-get install -y libgtk-3-dev devhelp
