#!/usr/bin/env bash

# create system user (no home dir, no login shell — it's not interactive)
sudo useradd -r -s /usr/bin/nologin kbdoverlay

# add it to the input group
sudo usermod -aG input kbdoverlay

# grant it access to your wayland socket (you'll need to redo this each login, or automate it)
setfacl -m u:kbdoverlay:rw /run/user/1000/$WAYLAND_DISPLAY
