cmake_minimum_required(VERSION 3.16)
project(CryptoQtApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Let CMake run moc / rcc automatically for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Use Qt5 (you have Qt 5.15.x installed)
find_package(Qt5 REQUIRED COMPONENTS Widgets)

# Try to find Crypto++ via pkg-config or fallback to linking -lcryptopp
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(CRYPTOPP QUIET cryptopp)
endif()

if(TARGET CryptoPP::CryptoPP)
  set(CRYPTOPP_TARGET CryptoPP::CryptoPP)
elseif(TARGET cryptopp)
  set(CRYPTOPP_TARGET cryptopp)
elseif(CRYPTOPP_FOUND)
  include_directories(${CRYPTOPP_INCLUDE_DIRS})
  link_libraries(${CRYPTOPP_LIBRARIES})
  set(CRYPTOPP_TARGET ${CRYPTOPP_LIBRARIES})
else()
  # fallback (common on many distros)
  set(CRYPTOPP_TARGET cryptopp)
endif()

set(SRCS
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
)

# Qt5 resource helper
# qt5_add_resources(QT_RESOURCES src/resources.qrc)
# add_executable(${PROJECT_NAME} ${SRCS} ${QT_RESOURCES})

# removed resources.qrc
add_executable(${PROJECT_NAME} ${SRCS})

target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets ${CRYPTOPP_TARGET})
