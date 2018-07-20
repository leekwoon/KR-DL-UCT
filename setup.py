import glob
import os

from setuptools import setup, Extension
from Cython.Build import cythonize
import numpy as np

EXTRA_COMPILE_ARGS = []

if os.name == "posix":
    EXTRA_COMPILE_ARGS += "-std=c++14".split(" ")

def is_cpp(fname):
    return fname.strip()[-4:] == ".cpp"

def walk_all_cpps(root):
    cpps = []
    for (dirpath, dirname, files) in os.walk(root):
        for file_name in files:
            if is_cpp(file_name):
                cpps.append(os.path.join(dirpath, file_name))
    return cpps

def define_extensions(**kwargs):
    client_ext = [
        Extension(
            name="src.env.curling_env",
            sources=walk_all_cpps("third_party/Box2D/Box2D") + [
                "src/env/curling_env.pyx",
                "src/env/CurlingSimulator.cpp"
            ],
            extra_compile_args=EXTRA_COMPILE_ARGS,
            libraries=[],
            extra_link_args=[],
            include_dirs=[
                "third_party/Box2D",
            ],
            language="c++"
        ), 
        Extension( 
            name="src.kr_dl_uct.player",
            sources=[
                "src/kr_dl_uct/player.pyx",
                "src/kr_dl_uct/node.cpp",
                "src/kr_dl_uct/kde.cpp"
            ],
            extra_compile_args=[],
            libraries=[],
            extra_link_args=[],
            include_dirs=[],
            language="c++"
        ),
    ]
    return cythonize(client_ext)

setup(
    # name=NAME,
    # version=VERSION,
    description="KR-DL-UCT for the game of simulated curling",
    author="SAIL",
    author_email="leekwoon@unist.ac.kr",
    #license="",
    install_requires=[
        "cython",
    ],
    # keywords="",
    ext_modules=define_extensions(),
)