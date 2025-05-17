FROM ubuntu:22.04
RUN apt update && apt install -y fuse libfuse-dev gcc
COPY antink.c /antink.c
RUN gcc -o /antink /antink.c -D_FILE_OFFSET_BITS=64 -lfuse
RUN mkdir -p /antink_mount
CMD ["/antink", "/antink_mount"]
