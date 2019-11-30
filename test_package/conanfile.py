from conans import ConanFile, CMake
import os

channel = os.getenv('CONAN_CHANNEL', 'inexor')
username = os.getenv('CONAN_USERNAME', 'testing')

class TestConan(ConanFile):
    settings = 'os', 'compiler', 'build_type', 'arch'
    requires = 'cppgraphqlgen/3.0.4@%s/%s' % (username, channel)
    generators = 'cmake'

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_dir=self.source_folder, build_dir='./')
        cmake.build()

    def imports(self):
        self.copy('*', 'bin', 'bin')

    def test(self):
        os.chdir('bin')
        self.run('.%ssample' % os.sep)
