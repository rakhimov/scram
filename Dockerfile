FROM ubuntu:16.04
RUN apt-get update
RUN apt-get install -y --no-install-recommends \
    texlive texlive-fonts-recommended texlive-latex-extra graphviz
RUN apt-get install -y git zip doxygen wget python python-pip python-setuptools
RUN pip install sphinx sphinx_rtd_theme lizard cppdep

ADD . scram/
WORKDIR scram
RUN mkdir -p build
CMD make build/dep_report.txt scram_core.dot 2> /dev/null && \
    make doxygen html
