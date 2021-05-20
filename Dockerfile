FROM archlinux

COPY scripts/mirrorlist /etc/pacman.d/
RUN pacman -Sy
RUN pacman --noconfirm -S git
RUN pacman --noconfirm -S gcc
RUN pacman --noconfirm -S make
RUN pacman --noconfirm -S cmake
RUN pacman --noconfirm -S python
RUN pacman --noconfirm -S python-pip
RUN pacman --noconfirm -S python-pybind11
RUN pacman --noconfirm -S python-numpy
RUN pacman --noconfirm -S qt5-base
RUN pacman --noconfirm -S libglvnd
RUN pacman --noconfirm -S mesa
RUN pacman --noconfirm -S ttf-roboto

RUN pip config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple
RUN pip install -U pip
RUN pip install PyQt5

RUN echo git clone https://gitee.com/archibate/zeno.git --depth=1 > /root/get-zeno.sh && chmod +x /root/get-zeno.sh

ENTRYPOINT bash
