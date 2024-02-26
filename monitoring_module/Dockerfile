FROM ubuntu:18.04
ADD cmake/build/monitoring-module /usr/bin/monitoring-module

COPY shared_library/libpthread.so.0 /lib/x86_64-linux-gnu/
COPY shared_library/libstdc++.so.6 /usr/lib/x86_64-linux-gnu/
COPY shared_library/libgcc_s.so.1 /lib/x86_64-linux-gnu/
COPY shared_library/libc.so.6 /lib/x86_64-linux-gnu/
COPY shared_library/libm.so.6 /lib/x86_64-linux-gnu/

EXPOSE 40203
WORKDIR /usr/bin
ENTRYPOINT ["/usr/bin/monitoring-module"]