FROM ubuntu:18.04
ENV BUILD_PACKAGES \
    make cmake g++ libxml2-dev libgoogle-perftools-dev libboost-program-options-dev libboost-math-dev libboost-random-dev libboost-filesystem-dev libboost-date-time-dev qt5-default libqt5gui5 libqt5opengl5 libqt5concurrent5 libqt5printsupport5 libqt5svg5-dev qttools5-dev-tools qttools5-dev libqt5opengl5-dev
ENV RUNTIME_PACKAGES \
    libxml2 libboost-filesystem1.65.1 libboost-program-options1.65.1 \
    libtcmalloc-minimal4
ADD . scram/
RUN apt-get update && \
    apt-get install -y --no-install-recommends $BUILD_PACKAGES
RUN cd scram && mkdir -p build && cd build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=ON && make install 
RUN cd ../.. && rm -rf ./scram
RUN apt-get remove --purge -y $BUILD_PACKAGES $(apt-mark showauto) && \
    apt-get install -y --no-install-recommends $RUNTIME_PACKAGES
RUN rm -rf /var/lib/apt/lists/*
#ENTRYPOINT ["scram"]
