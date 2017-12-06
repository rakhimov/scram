FROM alpine:edge
RUN apk add --update --no-cache g++ make cmake binutils boost-dev jemalloc-dev \
    libxml2-dev
ADD . scram/
WORKDIR scram
RUN mkdir -p build
RUN cd build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=OFF -DBUILD_TESTS=OFF \
    -DBUILD_SHARED_LIBS=OFF -DINSTALL_LIBS=OFF
RUN make install
ENTRYPOINT ["scram"]
