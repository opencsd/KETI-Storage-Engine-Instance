FROM ubuntu:18.04
ADD /cmake/build/merging-module /usr/bin/merging-module

COPY ./shared_library /usr/lib/x86_64-linux-gnu

EXPOSE 40202 40204
ENTRYPOINT ["/usr/bin/merging-module"]