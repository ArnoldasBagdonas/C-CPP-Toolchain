# syntax=docker/dockerfile:1.7
FROM debian:bookworm-slim

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# ------------------------------------------------------------------------------
# Global environment
# ------------------------------------------------------------------------------
ENV DEBIAN_FRONTEND=noninteractive \
    LANG=C.UTF-8 \
    LC_ALL=C.UTF-8 \
    _JAVA_OPTIONS="-Djava.awt.headless=true" \
    VIRTUAL_ENV=/opt/venv \
    PATH="/opt/venv/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"

ARG CLANG_VER=16

# ------------------------------------------------------------------------------
# Base system & common utilities
# ------------------------------------------------------------------------------
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        wget \
        gnupg \
        unzip \
        dos2unix \
        debconf-utils \
        software-properties-common \
        build-essential \
        ninja-build \
        gdb \
        cmake \
        git \
        python3 \
        python3-venv \
        python3-pip \
        cppcheck \
        valgrind \
        libboost-all-dev \
        passwd \
        lsb-release \
        sudo \
        fontconfig \
        xfonts-utils \
        cabextract \
        tree \
        gnuplot \
        default-jre \
        python3-pytest \
        lcov \
    && rm -rf /var/lib/apt/lists/*

# ------------------------------------------------------------------------------
# Documentation & LaTeX tooling
# ------------------------------------------------------------------------------
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        doxygen \
        doxygen-latex \
        graphviz \
        latexmk \
        texlive-latex-base \
        texlive-fonts-recommended \
        texlive-xetex \
        xindy \
    && rm -rf /var/lib/apt/lists/*

# ------------------------------------------------------------------------------
# Fonts (Debian)
# ------------------------------------------------------------------------------
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        fonts-cantarell \
        fonts-roboto \
        fonts-freefont-otf \
    && rm -rf /var/lib/apt/lists/*

# ------------------------------------------------------------------------------
# Ubuntu font (manual install)
# ------------------------------------------------------------------------------
RUN mkdir -p /usr/share/fonts/truetype/ubuntu && \
    wget -q https://assets.ubuntu.com/v1/0cef8205-ubuntu-font-family-0.83.zip \
        -O /tmp/ubuntu-font.zip && \
    unzip -q /tmp/ubuntu-font.zip -d /usr/share/fonts/truetype/ubuntu && \
    chmod -R a+rX /usr/share/fonts/truetype/ubuntu && \
    rm /tmp/ubuntu-font.zip && \
    fc-cache -f

# ------------------------------------------------------------------------------
# Microsoft Core Fonts (non-free)
# ------------------------------------------------------------------------------
# RUN printf "Types: deb\nURIs: http://deb.debian.org/debian\nSuites: bookworm bookworm-updates\nComponents: main contrib non-free\nSigned-By: /usr/share/keyrings/debian-archive-keyring.gpg\n\nTypes: deb\nURIs: http://security.debian.org/debian-security\nSuites: bookworm-security\nComponents: main contrib non-free\nSigned-By: /usr/share/keyrings/debian-archive-keyring.gpg\n" \
#     > /etc/apt/sources.list.d/non-free.sources && \
#     apt-get update && \
#     echo "ttf-mscorefonts-installer msttcorefonts/accepted-mscorefonts-eula select true" \
#         | debconf-set-selections && \
#     apt-get install -y --no-install-recommends ttf-mscorefonts-installer && \
#     fc-cache -f && \
#     rm -rf /var/lib/apt/lists/*

RUN mkdir -p /usr/share/fonts/truetype/msttcorefonts && \
    cd /usr/share/fonts/truetype/msttcorefonts && \
    wget -q https://sourceforge.net/projects/corefonts/files/the%20fonts/final/andale32.exe && \
    wget -q https://sourceforge.net/projects/corefonts/files/the%20fonts/final/arial32.exe && \
    wget -q https://sourceforge.net/projects/corefonts/files/the%20fonts/final/arialb32.exe && \
    wget -q https://sourceforge.net/projects/corefonts/files/the%20fonts/final/comic32.exe && \
    wget -q https://sourceforge.net/projects/corefonts/files/the%20fonts/final/courie32.exe && \
    wget -q https://sourceforge.net/projects/corefonts/files/the%20fonts/final/georgi32.exe && \
    wget -q https://sourceforge.net/projects/corefonts/files/the%20fonts/final/impact32.exe && \
    wget -q https://sourceforge.net/projects/corefonts/files/the%20fonts/final/times32.exe && \
    wget -q https://sourceforge.net/projects/corefonts/files/the%20fonts/final/trebuc32.exe && \
    wget -q https://sourceforge.net/projects/corefonts/files/the%20fonts/final/verdan32.exe && \
    fc-cache -f

# ------------------------------------------------------------------------------
# PlantUML
# ------------------------------------------------------------------------------
WORKDIR /opt/plantuml
RUN wget -q https://netcologne.dl.sourceforge.net/project/plantuml/plantuml.jar

RUN echo 'alias plantuml="java -jar /opt/plantuml/plantuml.jar"' \
    >> /etc/bash.bashrc

# ------------------------------------------------------------------------------
# LLVM / Clang
# ------------------------------------------------------------------------------
WORKDIR /tmp
RUN wget -q https://apt.llvm.org/llvm.sh && \
    chmod +x llvm.sh && \
    ./llvm.sh ${CLANG_VER} all && \
    rm llvm.sh && \
    ln -sf /usr/bin/clang-${CLANG_VER} /usr/bin/clang && \
    ln -sf /usr/bin/clang-tidy-${CLANG_VER} /usr/bin/clang-tidy && \
    ln -sf /usr/bin/clang-format-${CLANG_VER} /usr/bin/clang-format && \
    ln -sf /usr/bin/scan-build-${CLANG_VER} /usr/bin/scan-build && \
    ln -sf /usr/bin/run-clang-tidy-${CLANG_VER} /usr/bin/run-clang-tidy && \
    ln -sf /usr/bin/clang-apply-replacements-${CLANG_VER} /usr/bin/clang-apply-replacements && \
    ln -sf /usr/bin/llvm-cov-${CLANG_VER} /usr/bin/llvm-cov && \
    ln -sf /usr/bin/llvm-profdata-${CLANG_VER} /usr/bin/llvm-profdata && \
    ln -sf /usr/bin/llvm-symbolizer-${CLANG_VER} /usr/bin/llvm-symbolizer

# ------------------------------------------------------------------------------
# Python virtual environment
# ------------------------------------------------------------------------------
COPY requirements.txt /tmp/requirements.txt
RUN python3 -m venv ${VIRTUAL_ENV} && \
    pip install --no-cache-dir -r /tmp/requirements.txt && \
    rm /tmp/requirements.txt

# ------------------------------------------------------------------------------
# Non-root user
# ------------------------------------------------------------------------------
RUN adduser --disabled-password --gecos "" vscode && \
    usermod -aG sudo vscode && \
    echo "vscode ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/vscode && \
    chmod 0440 /etc/sudoers.d/vscode

USER vscode
WORKDIR /home/vscode

# ------------------------------------------------------------------------------
# Entrypoint
# ------------------------------------------------------------------------------
ENTRYPOINT ["/bin/bash"]
