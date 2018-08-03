#!/bin/sh

TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
SRCDIR=${SRCDIR:-$TOPDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

MASSGRIDD=${MASSGRIDD:-$SRCDIR/massgridd}
MASSGRIDCLI=${MASSGRIDCLI:-$SRCDIR/massgrid-cli}
MASSGRIDTX=${MASSGRIDTX:-$SRCDIR/massgrid-tx}
MASSGRIDQT=${MASSGRIDQT:-$SRCDIR/qt/massgrid-qt}

[ ! -x $MASSGRIDD ] && echo "$MASSGRIDD not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
MGDVER=($($MASSGRIDCLI --version | head -n1 | awk -F'[ -]' '{ print $6, $7 }'))

# Create a footer file with copyright content.
# This gets autodetected fine for massgridd if --version-string is not set,
# but has different outcomes for massgrid-qt and massgrid-cli.
echo "[COPYRIGHT]" > footer.h2m
$MASSGRIDD --version | sed -n '1!p' >> footer.h2m

for cmd in $MASSGRIDD $MASSGRIDCLI $MASSGRIDTX $MASSGRIDQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${MGDVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${MGDVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
