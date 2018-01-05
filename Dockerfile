FROM ubuntu:17.10
ENV BUILD_PACKAGES \
    make cmake g++ libxml2-dev \
    libgoogle-perftools-dev libboost-program-options-dev \
    libboost-math-dev libboost-random-dev libboost-filesystem-dev \
    libboost-date-time-dev
ENV RUNTIME_PACKAGES \
    libxml2 libboost-filesystem1.62.0 libboost-program-options1.62.0 \
    libtcmalloc-minimal4
ADD . scram/
RUN apt-get update && \
    apt-get install -y --no-install-recommends $BUILD_PACKAGES && \
    cd scram && mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=OFF && make install && \
    cd ../.. && rm -rf ./scram && \
    apt-get remove --purge -y $BUILD_PACKAGES $(apt-mark showauto) && \
    apt-get install -y --no-install-recommends $RUNTIME_PACKAGES && \
    rm -rf /var/lib/apt/lists/*
ENTRYPOINT ["scram"]
