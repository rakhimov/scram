FROM alpine:edge
RUN apk add --update --no-cache g++ make cmake binutils boost-dev jemalloc-dev
RUN apk add libxml++-2.6-dev --update --no-cache \
    --repository http://dl-3.alpinelinux.org/alpine/edge/testing/ --allow-untrusted
ADD . scram/
WORKDIR scram
RUN mkdir -p build
RUN python install.py --release --prefix=/usr -DBUILD_GUI=OFF -DBUILD_TESTS=OFF \
    -DBUILD_SHARED_LIBS=OFF -DINSTALL_LIBS=OFF
ENTRYPOINT ["scram"]
