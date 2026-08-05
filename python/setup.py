# Metadata lives here (not [project]) because the oldest supported
# setuptools must still build this. Platform wheel despite no extension
# module: the package ships a prebuilt shared library + stanc binary.
from setuptools import setup
from setuptools.dist import Distribution


class BinaryDistribution(Distribution):
    def has_ext_modules(self):
        return True


setup(
    name="stanrt",
    version="0.1.0.dev0",
    description=("Portable Stan runtime: compile and sample Stan models "
                 "with no C++ toolchain"),
    long_description=open("README.md").read(),
    long_description_content_type="text/markdown",
    python_requires=">=3.9",
    install_requires=["numpy>=1.22"],
    packages=["stanrt"],
    package_data={"stanrt": ["_bin/*"]},
    include_package_data=True,
    distclass=BinaryDistribution,
)
