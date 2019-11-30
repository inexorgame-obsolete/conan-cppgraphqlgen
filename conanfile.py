from conans import ConanFile, CMake, tools
import os

PATCH_FOR_3_0_4 = '''From c467ebb380feffa502aa0643ee63421dcadd988c Mon Sep 17 00:00:00 2001
From: a_teammate <madoe3@web.de>
Date: Sat, 30 Nov 2019 18:19:32 +0100
Subject: [PATCH] make cmakelists.txt use conan

---
 CMakeLists.txt     |  6 ++++--
 src/CMakeLists.txt | 16 ++++------------
 2 files changed, 8 insertions(+), 14 deletions(-)

diff --git a/CMakeLists.txt b/CMakeLists.txt
index 14970b2..6ac01f7 100644
--- a/CMakeLists.txt
+++ b/CMakeLists.txt
@@ -3,6 +3,8 @@
 
 cmake_minimum_required(VERSION 3.8.2)
 project(cppgraphqlgen VERSION 3.0.0)
+include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
+            conan_basic_setup(TARGETS)
 
 set(CMAKE_CXX_STANDARD 17)
 
@@ -23,7 +25,7 @@ function(add_bigobj_flag target)
     target_compile_options(${target} PRIVATE /bigobj)
   endif()
 endfunction()
-
+include_directories("include", "PEGTL/include")
 find_package(Threads MODULE REQUIRED)
 
 find_package(pegtl 3.0.0 QUIET CONFIG)
@@ -38,7 +40,7 @@ endif()
 
 add_subdirectory(src)
 
-option(GRAPHQL_UPDATE_SAMPLES "Regenerate the sample schema sources whether or not we're building the tests." ON)
+option(GRAPHQL_UPDATE_SAMPLES "Regenerate the sample schema sources whether or not we're building the tests." OFF)
 
 if(GRAPHQL_BUILD_TESTS OR GRAPHQL_UPDATE_SAMPLES)
   add_subdirectory(samples)
diff --git a/src/CMakeLists.txt b/src/CMakeLists.txt
index 8161ab3..8f21d05 100644
--- a/src/CMakeLists.txt
+++ b/src/CMakeLists.txt
@@ -32,18 +32,12 @@ if(GRAPHQL_BUILD_SCHEMAGEN)
     $<TARGET_OBJECTS:graphqlresponse>
     SchemaGenerator.cpp)
   target_link_libraries(schemagen PRIVATE graphqlpeg)
-  
-  set(BOOST_COMPONENTS program_options)
-  set(BOOST_LIBRARIES Boost::program_options)
-  
+
   if(NOT MSVC)
-    set(BOOST_COMPONENTS ${BOOST_COMPONENTS} filesystem)
-    set(BOOST_LIBRARIES ${BOOST_LIBRARIES} Boost::filesystem)
     target_compile_options(schemagen PRIVATE -DUSE_BOOST_FILESYSTEM)
+    target_link_libraries(schemagen PRIVATE CONAN_PKG::boost_filesystem)
   endif()
-  
-  find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})
-  target_link_libraries(schemagen PRIVATE ${BOOST_LIBRARIES})
+  target_link_libraries(schemagen PRIVATE CONAN_PKG::boost_program_options)
 
   install(TARGETS schemagen
     EXPORT cppgraphqlgen-targets
@@ -93,11 +87,9 @@ target_include_directories(graphqlservice PRIVATE
 option(GRAPHQL_USE_RAPIDJSON "Use RapidJSON for JSON serialization." ON)
 
 if(GRAPHQL_USE_RAPIDJSON)
-  find_package(RapidJSON CONFIG REQUIRED)
-
   set(BUILD_GRAPHQLJSON ON)
   add_library(graphqljson JSONResponse.cpp)
-  target_include_directories(graphqljson SYSTEM PRIVATE ${RAPIDJSON_INCLUDE_DIRS})
+  target_include_directories(graphqljson SYSTEM PRIVATE ${CONAN_INCLUDE_DIRS_RAPIDJSON})
 endif()
 
 # graphqljson
-- 
2.7.4
'''


class CppGraphQLGenConan(ConanFile):

    name = 'cppgraphqlgen'
    version = '3.0.4'
    commit = "v{}".format(version)
    license = 'MIT'
    description = 'C++ GraphQL schema service generator'
    url = 'https://github.com/microsoft/cppgraphqlgen'
    settings = 'os', 'compiler', 'build_type', 'arch'
    options = {"build_schemagen": [True, False]}

    build_requires = (
        "boost_program_options/1.69.0@bincrafters/stable",
        "boost_filesystem/1.69.0@bincrafters/stable",
        "rapidjson/1.1.0@bincrafters/stable"
    )

    generators = "cmake"

    default_options = { "build_schemagen": True }

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def source(self):
        self.run('git clone --recursive -b {commit} --depth 1 {url}'.format(commit=self.commit,
                                                                url=self.url))
        tools.patch(base_path=self.name, patch_string=PATCH_FOR_3_0_4)

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_dir=self.name, build_dir='./',
                        args=["-DGRAPHQL_BUILD_TESTS=OFF",
                              "-DGRAPHQL_UPDATE_SAMPLES=OFF",
                              "-DGRAPHQL_BUILD_SCHEMAGEN={}".format("ON" if self.options.build_schemagen else "OFF")])
        cmake.build()

    def package(self):
        self.copy('graphqlservice/*', dst='include', src='include')
        self.copy('graphqlservice/*', dst='include', src=os.path.join(self.name, 'include'))
        self.copy('tao/*', dst='include', src=os.path.join(self.name, 'PEGTL', 'include'))
        self.copy('*graphqlpeg.*', dst='lib', src='lib')
        self.copy('*graphqlservice.*', dst='lib', src='lib')
        self.copy('*graphqljson.*', dst='lib', src='lib')
        if self.options.build_schemagen:
            self.copy('schemagen*', dst='bin', src='bin')

    def package_info(self):
        self.cpp_info.libs = ['graphqlservice', 'graphqljson', 'graphqlpeg']
        self.cpp_info.includedirs = ['include']

