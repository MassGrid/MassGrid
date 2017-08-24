#!/bin/bash
# Mlgb wallet startup scripts.
# Created by jesse
# 2017-08-24

BINPATH="/usr/local/bin"

case $1 in 
    start|begin) 
        echo "Start mlgbcoin wallet server." 
        ${BINPATH}/mlgbcoind -rpcbind=0.0.0.0 \
                             -rpcport=9442 \
                             -rpcallowip=192.168.0.0/16 \
                             --daemon

        ;; 
    stop|end) 
        echo "Stop mlgbcoin wallet server."
        ${BINPATH}/mlgbcoin-cli stop
        ;; 
    *) 
        echo "Usage:/bin/sh ${0} {start|stop}" 
        ;; 
esac 
