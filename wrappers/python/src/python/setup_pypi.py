import os
import sys
from pprint import pprint

from setuptools import setup, find_packages

from setuptools.command.install import install
from distutils.command.build import build

sys.path.insert(0, '@CMAKE_SOURCE_DIR@')
import versioneer

dyn_lib_extensions = [".so", ".dll", ".lib", ".dylib"]
print_sep = "***" * 15

__version__ = '@READDY_VERSION@'


class ReaDDyBuild(build):
    @staticmethod
    def is_dynlib(file_path):
        for ext in dyn_lib_extensions:
            if file_path.endswith(ext):
                return True
        return False

    def run(self):
        # super build
        build.run(self)

        # current file
        file_path = os.path.realpath(__file__)
        print("\trealpath: %s" % file_path)
        file_dir = os.path.dirname(__file__)
        print("\tdirname: %s" % file_dir)

        target_files = []

        for curr_dir, curr_subdirs, curr_files in os.walk(os.path.join(file_dir, "readdy")):
            print("\twalking: %s" % curr_dir)
            for f in curr_files:
                if self.is_dynlib(f):
                    print("\t\tfound dynlib %s" % f)
                    target_files.append(os.path.join(curr_dir, f))
        print("\tdynlibs: %s" % target_files)

        # copy resulting tool to library build folder
        internal_build = os.path.join(self.build_lib, "readdy", "_internal")
        self.mkpath(internal_build)

        if not self.dry_run:
            for target in target_files:
                self.copy_file(target, internal_build)


class ReaDDyInstall(install):
    def run(self):
        # run original install code
        install.run(self)

        # install libs
        internal_build = os.path.join(self.build_lib, "readdy", "_internal")
        internal_target = os.path.join(self.install_lib, "readdy", "_internal")
        print("\t setup.py: installing from %s to %s" % (internal_build, internal_target))
        self.copy_tree(internal_build, internal_target)


def get_package_dir():
    return os.path.join('@CMAKE_CURRENT_SOURCE_DIR@', "src", "python")


cmdclass = versioneer.get_cmdclass()
cmdclass['build'] = ReaDDyBuild
cmdclass['install'] = ReaDDyInstall

metadata = dict(
    name='ReaDDy',
    version=versioneer.get_version(),
    package_dir={'': get_package_dir()},
    package_data={'readdy._internal': ["*"]},
    packages=find_packages(where=get_package_dir()),
    cmdclass=cmdclass
)

if __name__ == '__main__':
    print("%s python setup start %s" % (print_sep, print_sep))
    print("calling setup with metadata:")
    pprint(metadata, indent=2, width=2)
    setup(**metadata)
    print("%s python setup end %s" % (print_sep, print_sep))
