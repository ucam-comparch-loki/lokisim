FROM ubuntu:latest

RUN apt-get update && apt-get install -y build-essential git libsystemc-dev

RUN git clone -b accelerator --single-branch \
    https://github.com/ucam-comparch-loki/lokisim.git

WORKDIR /lokisim/build

RUN make -j8

WORKDIR /lokisim/bin

ENTRYPOINT ["./lokisim"]