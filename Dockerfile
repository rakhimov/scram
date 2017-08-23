FROM alpine:edge
RUN apk add --update --no-cache g++ make cmake binutils boost-dev jemalloc-dev \
    libxml2-dev
ADD . scram/
WORKDIR scram
RUN mkdir -p build
RUN python install.py --release --prefix=/usr -DBUILD_GUI=OFF -DBUILD_TESTS=OFF \
    -DBUILD_SHARED_LIBS=OFF -DINSTALL_LIBS=OFF
ENTRYPOINT ["scram"]
