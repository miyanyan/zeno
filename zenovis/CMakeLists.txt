file(GLOB_RECURSE source CONFIGURE_DEPENDS include/*.h src/*.cpp)
file(GLOB_RECURSE glad_source CONFIGURE_DEPENDS glad/include/*.h glad/src/*.c)
file(GLOB_RECURSE stbi_source CONFIGURE_DEPENDS stbi/include/*.h stbi/src/*.cpp stbi/src/*.c)

#OPTION(OPTIX_USE_20XX "turn on if on a 20xx gpu" OFF)

add_library(zenovis OBJECT ${source} ${glad_source} ${stbi_source})
target_link_libraries(zenovis PRIVATE ${CMAKE_DL_LIBS})
target_link_libraries(zenovis PUBLIC zeno)
target_include_directories(zenovis PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/glad/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/stbi/include>)

option(ZENO_ENABLE_ZHXXVIS "Enable Zhxx Real-time Rendering Engine" ON)
if (ZENO_ENABLE_ZHXXVIS)
    add_subdirectory(zhxxvis)
    target_compile_definitions(zenovis PRIVATE -DZENO_ENABLE_ZHXXVIS)
endif()

if (ZENO_DEBUG_MSVC)
    zeno_dbg_msvc(zenovis)
endif()

option(ZENO_ENABLE_OPTIX "Enable NVIDIA OptiX in ZENO for path tracing" OFF)
if (ZENO_ENABLE_OPTIX)
    add_subdirectory(xinxinoptix)  # this in-place modify zenovis
    target_compile_definitions(zenovis PRIVATE -DZENO_ENABLE_OPTIX)
    target_include_directories(zenovis PRIVATE xinxinoptix)
endif()

# the check of cuda display device fails on some user env, and the 
# reasons are still unknown, but it seems that we can skip it.
option(ZENO_SKIP_CUDA_DISPLAY_CHECK "Skip CUDA Device Check" OFF)
if (ZENO_SKIP_CUDA_DISPLAY_CHECK)
    target_compile_definitions(zenovis PRIVATE -DZENO_SKIP_CUDA_DISPLAY_CHECK)
endif()

if (ZENO_ENABLE_OPENMP)
    find_package(OpenMP)
    if (TARGET OpenMP::OpenMP_CXX)
        message(STATUS "Found OpenMP::OpenMP_CXX")
        target_link_libraries(zenovis PRIVATE OpenMP::OpenMP_CXX)
        if (MSVC)
            target_compile_options(zenovis PRIVATE /openmp:experimental /openmp:llvm)
        endif()
    else()
        message(WARNING "Not found OpenMP")
    endif()
endif()

#if (ZENO_INSTALL_TARGET)
    #install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include DESTINATION include/Zeno/zenovis)
    #install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/glad/include DESTINATION include/Zeno/zenovis)
    #install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/stbi/include DESTINATION include/Zeno/zenovis)
    #install(TARGETS zenovis EXPORT ZenoTargets)
#endif()
