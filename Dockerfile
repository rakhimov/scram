FROM alpine:edge
RUN apk add --update --no-cache g++ make cmake binutils boost-dev jemalloc-dev \
    libxml2-dev
ADD . scram/
RUN cd scram && mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=OFF && make install
RUN rm -rf scram /var/cache/*
ENTRYPOINT ["scram"]
