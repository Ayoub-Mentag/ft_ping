# Dockerfile for setting up the project environment

FROM debian:buster

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Replace sources.list with archive.debian.org entries for buster
RUN sed -i 's|http://deb.debian.org/debian|http://archive.debian.org/debian|g' /etc/apt/sources.list \
  && echo 'Acquire::Check-Valid-Until "false";' > /etc/apt/apt.conf.d/99no-check-valid-until

# Install dependencies and remove iputils-ping
RUN apt-get update \
  && apt-get remove -y iputils-ping || true \
  && apt-get install -y --no-install-recommends \
        apt-utils \
        ca-certificates \
        make \
        build-essential \
        wget \
        tar \
        netbase \
  && rm -rf /var/lib/apt/lists/*

# Build and install inetutils 2.0 (for ping)
WORKDIR /tmp
RUN wget http://ftp.gnu.org/gnu/inetutils/inetutils-2.0.tar.gz \
  && tar xzf inetutils-2.0.tar.gz \
  && cd inetutils-2.0 \
  && ./configure --disable-servers \
  && make \
  && make install \
  && cd .. \
  && rm -rf inetutils-2.0*

# Create the ft_ping directory and set it as the working directory
WORKDIR /
VOLUME /ft_ping
WORKDIR /ft_ping

COPY . ft_ping

CMD ["/bin/bash"]