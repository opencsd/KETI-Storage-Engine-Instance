FROM ubuntu:18.04
# ADD /cmake/build/offloading-container /usr/bin/offloading-container
COPY /cmake/build/storage-engine-interface /usr/bin/storage-engine-interface

COPY ./shared_library /usr/lib/x86_64-linux-gnu

EXPOSE 40201 
ENTRYPOINT ["/usr/bin/storage-engine-interface"]