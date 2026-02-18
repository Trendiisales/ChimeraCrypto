cmake_minimum_required(VERSION 3.15)
project(ChimeraCrypto CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mavx2 -DNDEBUG -flto -ffast-math")
include_directories(include)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_library(LIBWEBSOCKETS_LIBRARY websockets)
find_package(CURL REQUIRED)

add_executable(chimera
    src/main.cpp
    src/InstitutionalOrderBook.cpp
    src/DepthManager.cpp
    src/SignalLatch.cpp
    src/l2/L2Book.cpp
    src/l2/L2Bootstrapper.cpp
    src/l2/SnapshotFetcher.cpp
    src/microstructure/ImbalanceAcceleration.cpp
    src/microstructure/CancelBurstDetector.cpp
    src/microstructure/SpreadCollapsePredictor.cpp
    src/microstructure/MakerTakerPolicy.cpp
    src/microstructure/LiquidityCorrelationEngine.cpp
    src/risk/AdaptiveSizer.cpp
    src/regime/VolatilityRegimeClassifier.cpp
)

target_link_libraries(chimera
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
    ${LIBWEBSOCKETS_LIBRARY}
    CURL::libcurl
    z
)
