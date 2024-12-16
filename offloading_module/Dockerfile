FROM ubuntu:18.04
ADD /cmake/build/offloading-module /usr/bin/offloading-module
# COPY /cmake/build/offloading-module /usr/bin/offloading-module

COPY ./shared_library /usr/lib/x86_64-linux-gnu
VOLUME /mnt/gluster

EXPOSE 40201 
ENTRYPOINT ["/usr/bin/offloading-module"]