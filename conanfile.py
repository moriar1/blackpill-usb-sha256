from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.cmake import CMake, cmake_layout


class BlackpillUsart(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "CMakeToolchain"

    def validate(self):
        if self.settings.os != "baremetal" or self.settings.arch != "armv7":
            raise ConanInvalidConfiguration(
                "This projecy can only be build for STM32F4XX board which is not "
                f"{self.settings.os}-{self.settings.arch}"
            )

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.30]")

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure(variables={"CMAKE_TRY_COMPILE_TARGET_TYPE": "STATIC_LIBRARY"})
        cmake.build()
