# syntax=docker/dockerfile:1.4

FROM ubuntu:22.04 as builder
RUN apt update && apt install -y cmake git gcc-arm-none-eabi python3 ninja-build
CMD [ "/bin/sh", "-c", "while sleep 1000; do :; done" ]