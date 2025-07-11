FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y \
    build-essential \
    curl \
    tar \
    bzip2 \
    && apt-get clean && \
    rm -rf /var/lib/apt/lists/*

ARG BOOST_VERSION=1.88.0
ARG BOOST_TAR=boost_${BOOST_VERSION//./_}.tar.bz2
ARG BOOST_URL=https://archives.boost.io/release/${BOOST_VERSION}/source/${BOOST_TAR}

WORKDIR /tmp/boost
RUN curl -sSL ${BOOST_URL} | tar --bzip2 -xf - && \
    mv boost_* boost_src && \
    cd boost_src && \
    ./bootstrap.sh --prefix=/usr/local && \
    ./b2 install -j$(nproc) --with-system --with-filesystem --with-thread link=static runtime-link=static