#!/usr/bin/env bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/..

DOCKER_IMAGE=${DOCKER_IMAGE:-massgridpay/massgridd-develop}
DOCKER_TAG=${DOCKER_TAG:-latest}

BUILD_DIR=${BUILD_DIR:-.}

rm docker/bin/*
mkdir docker/bin
cp $BUILD_DIR/src/massgridd docker/bin/
cp $BUILD_DIR/src/massgrid-cli docker/bin/
cp $BUILD_DIR/src/massgrid-tx docker/bin/
strip docker/bin/massgridd
strip docker/bin/massgrid-cli
strip docker/bin/massgrid-tx

docker build --pull -t $DOCKER_IMAGE:$DOCKER_TAG -f docker/Dockerfile docker
