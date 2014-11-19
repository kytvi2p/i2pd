#!/bin/sh

set -u
set -e

_HOME=$HOME/.i2pd
HOSTS=/usr/share/doc/i2pd/examples/hosts.txt.gz

[ -d "$_HOME" ] || mkdir "$_HOME"

if [ ! -r $_HOME/hosts.txt ]; then
    zcat $HOSTS > $_HOME/hosts.txt
fi

exec /usr/lib/i2pd/i2p "$@"
