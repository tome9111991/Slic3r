cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project (slic3r)

option(Enable_GUI "Use the wxWidgets code in slic3r.cpp" OFF)
option(GUI_BUILD_TESTS "Build tests for Slic3r GUI." OFF)
option(SLIC3R_BUILD_TESTS "Build tests for libslic3r." OFF)
option(SLIC3R_STATIC "Build and link Slic3r statically." ON)
option(BUILD_EXTRUDE_TIN "Build and link the extrude-tin application." OFF)
option(PROFILE "Build with gprof profiling output." OFF)
option(COVERAGE "Build with gcov code coverage profiling." OFF)
option(SLIC3R_DEBUG "Build with Slic3r's debug output" OFF)

# only on newer GCCs: -ftemplate-backtrace-limit=0
if(NOT MSVC)
    add_compile_options(-ftemplate-backtrace-limit=0)
endif()
add_compile_options(-DNO_PERL -DM_PI=3.14159265358979323846 -DHAS_BOOL -DBOOST_ASIO_DISABLE_KQUEUE)
# Define SLIC3R_CPPVER to 11 to enforce C++11 paths (e.g. using unique_ptr instead of auto_ptr)
add_compile_options(-DSLIC3R_CPPVER=23)

if(SLIC3R_DEBUG)
    add_compile_options(-DSLIC3R_DEBUG)
endif()

if (MSVC)
    add_compile_options(-W3)
    add_compile_options(-bigobj)
    # Enable standard C++ conformance (supports 'and', 'or', etc.)
    add_compile_options(/permissive-)
    # Define Windows version for Boost ASIO (0x0601 = Windows 7)
    add_compile_options(-D_WIN32_WINNT=0x0601)
    add_compile_options(-D_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(UNICODE _UNICODE)
else()
    add_compile_options(-Wall)
endif()

if(DEFINED ENV{SLIC3R_VAR_REL})
    add_compile_options(-DVAR_REL=$ENV{SLIC3R_VAR_REL})
endif(DEFINED ENV{SLIC3R_VAR_REL})

if(DEFINED ENV{SLIC3R_VAR_ABS})
    add_compile_options(-DVAR_ABS)
endif(DEFINED ENV{SLIC3R_VAR_ABS})

if(DEFINED ENV{SLIC3R_VAR_ABS_PATH})
    add_compile_options(-DVAR_ABS_PATH=$ENV{SLIC3R_VAR_ABS_PATH})
endif(DEFINED ENV{SLIC3R_VAR_ABS_PATH})

# mingw needs to be told to deal with large objects
if(MINGW)
    add_compile_options(-Wa,-mbig-obj) 
endif()

if(Enable_GUI) 
    add_compile_options(-DUSE_WX)
endif(Enable_GUI)

if($ENV{TRAVIS})
    if($ENV{TRAVIS} STREQUAL "true")
        message(STATUS "Building on Travis-CI.")
        set(IS_TRAVIS_BUILD TRUE)
    endif()
endif()

execute_process(COMMAND git rev-parse --short HEAD OUTPUT_VARIABLE GIT_VERSION ERROR_QUIET)
if (MSVC)
    if (CMAKE_BUILD_TYPE MATCHES Debug)
        add_compile_options(-DEBUG)
        add_compile_options(-Od)
        if (SLIC3R_STATIC)
            add_compile_options(-MTd)
        else(SLIC3R_STATIC)
            add_compile_options(-MDd)
        endif(SLIC3R_STATIC)
    elseif (CMAKE_BUILD_TYPE MATCHES Release)
        if (SLIC3R_STATIC)
            add_compile_options(-MT)
        else(SLIC3R_STATIC)
            add_compile_options(-MD)
        endif(SLIC3R_STATIC)
    elseif (CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
        add_compile_options(-DEBUG)
        add_compile_options(-O2)
    endif()
else()    
    if (CMAKE_BUILD_TYPE MATCHES Debug)
        add_compile_options(-g -O0)
    elseif (CMAKE_BUILD_TYPE MATCHES Release)
        add_compile_options(-O3)
    elseif (CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
        add_compile_options(-g -O3)
    endif()
endif()

if(PROFILE)
    add_compile_options(-g -pg -DBUILD_PROFILE)
endif(PROFILE)

if(COVERAGE)
    add_compile_options(-g -ftest-coverage)
endif(COVERAGE)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

IF(CMAKE_HOST_APPLE)
    add_compile_options(-stdlib=libc++ -DBOOST_THREAD_DONT_USE_CHRONO -DBOOST_NO_CXX11_RVALUE_REFERENCES -DBOOST_THREAD_USES_MOVE)
    set(CMAKE_EXE_LINKER_FLAGS "-framework IOKit -framework CoreFoundation -lc++")
ELSE(CMAKE_HOST_APPLE)
#    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -L.")
ENDIF(CMAKE_HOST_APPLE)


if(SLIC3R_STATIC)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME ON)
else(SLIC3R_STATIC)
    set(Boost_USE_STATIC_LIBS OFF)
    set(Boost_USE_STATIC_RUNTIME OFF)
endif(SLIC3R_STATIC)


if (NOT GIT_VERSION STREQUAL "")
    if (MSVC)
        add_compile_definitions(SLIC3R_BUILD_COMMIT=${GIT_VERSION})
    else(MSVC)
        execute_process(COMMAND git rev-parse --short HEAD OUTPUT_VARIABLE GIT_VERSION_1)
        string(REGEX REPLACE "\n$" "" GIT_VERSION "${GIT_VERSION_1}")
        add_compile_definitions(SLIC3R_BUILD_COMMIT=${GIT_VERSION})
    endif(MSVC)
endif(NOT GIT_VERSION STREQUAL "")

find_package(Threads REQUIRED)

set(Boost_NO_BOOST_CMAKE ON)
find_package(Boost REQUIRED COMPONENTS system thread filesystem)

if (NOT (${Boost_VERSION_STRING} VERSION_LESS "1.74.0"))
    MESSAGE("Adding in boost::nowide")
    find_package(boost_nowide CONFIG QUIET)
    if(boost_nowide_FOUND)
        message(STATUS "Found boost_nowide via CONFIG")
        set(BOOST_NOWIDE_FOUND TRUE)
        list(APPEND Boost_LIBRARIES Boost::nowide)
    else()
        find_package(Boost COMPONENTS nowide QUIET)
        if(Boost_NOWIDE_FOUND)
            message(STATUS "Found Boost::nowide via COMPONENTS")
            set(BOOST_NOWIDE_FOUND TRUE)
            list(APPEND Boost_LIBRARIES Boost::nowide)
        else()
            set(BOOST_NOWIDE_FOUND FALSE)
        endif()
    endif()
endif()

set(LIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/src/)
set(GUI_LIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/src/GUI/)

set(TESTDIR ${CMAKE_CURRENT_SOURCE_DIR}/src/test)
set(GUI_TESTDIR ${CMAKE_CURRENT_SOURCE_DIR}/src/test/GUI/)

# directory that contains the dependent non-source files, like models and configurations
set(TESTFILE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/test/inputs/)

set(EXPAT_INCLUDES
    ${LIBDIR}/expat
)
set(ADMESH_INCLUDES
    ${LIBDIR}/admesh
)

set(BSPLINE_INCLUDES
    ${LIBDIR}/BSpline/
)

set(POLY2TRI_INCLUDES
    ${LIBDIR}/poly2tri
    ${LIBDIR}/poly2tri/sweep
    ${LIBDIR}/poly2tri/common
)
set(COMMON_INCLUDES
    ${LIBDIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/standalone/
)
set(SLIC3R_INCLUDES
    ${COMMON_INCLUDES}
    ${EXPAT_INCLUDES}
    ${ADMESH_INCLUDES}
    ${POLY2TRI_INCLUDES}
    ${BSPLINE_INCLUDES}
)
set(LIBSLIC3R_INCLUDES
    ${LIBDIR}/libslic3r
    ${LIBDIR}/libslic3r/IO
)


add_library(ZipArchive STATIC
    ${LIBDIR}/Zip/ZipArchive.cpp
)

add_library(miniz STATIC
    ${LIBDIR}/miniz/miniz.c
)
target_compile_features(ZipArchive PUBLIC cxx_std_23)
target_include_directories(ZipArchive PUBLIC ${COMMON_INCLUDES})
target_compile_options(ZipArchive PRIVATE -w)

add_library(libslic3r STATIC
    ${LIBDIR}/libslic3r/BoundingBox.cpp
    ${LIBDIR}/libslic3r/BridgeDetector.cpp
    ${LIBDIR}/libslic3r/ClipperUtils.cpp
    ${LIBDIR}/libslic3r/ConfigBase.cpp
    ${LIBDIR}/libslic3r/Config.cpp
    ${LIBDIR}/libslic3r/ConditionalGCode.cpp
    ${LIBDIR}/libslic3r/ExPolygon.cpp
    ${LIBDIR}/libslic3r/ExPolygonCollection.cpp
    ${LIBDIR}/libslic3r/Extruder.cpp
    ${LIBDIR}/libslic3r/ExtrusionEntity.cpp
    ${LIBDIR}/libslic3r/ExtrusionEntityCollection.cpp
    ${LIBDIR}/libslic3r/Fill/Fill.cpp
    ${LIBDIR}/libslic3r/Fill/Fill3DHoneycomb.cpp
    ${LIBDIR}/libslic3r/Fill/FillConcentric.cpp
    ${LIBDIR}/libslic3r/Fill/FillHoneycomb.cpp
    ${LIBDIR}/libslic3r/Fill/FillPlanePath.cpp
    ${LIBDIR}/libslic3r/Fill/FillRectilinear.cpp
    ${LIBDIR}/libslic3r/Fill/FillGyroid.cpp
    ${LIBDIR}/libslic3r/Flow.cpp
    ${LIBDIR}/libslic3r/GCode.cpp
    ${LIBDIR}/libslic3r/PrintGCode.cpp
    ${LIBDIR}/libslic3r/GCode/CoolingBuffer.cpp
    ${LIBDIR}/libslic3r/GCode/SpiralVase.cpp
    ${LIBDIR}/libslic3r/GCodeReader.cpp
    ${LIBDIR}/libslic3r/GCodeSender.cpp
    ${LIBDIR}/libslic3r/GCodeTimeEstimator.cpp
    ${LIBDIR}/libslic3r/GCodeWriter.cpp
    ${LIBDIR}/libslic3r/Geometry.cpp
    ${LIBDIR}/libslic3r/IO.cpp
    ${LIBDIR}/libslic3r/IO/AMF.cpp
    ${LIBDIR}/libslic3r/IO/TMF.cpp
    ${LIBDIR}/libslic3r/Layer.cpp
    ${LIBDIR}/libslic3r/LayerRegion.cpp
    ${LIBDIR}/libslic3r/LayerRegionFill.cpp
    ${LIBDIR}/libslic3r/LayerHeightSpline.cpp
    ${LIBDIR}/libslic3r/Line.cpp
    ${LIBDIR}/libslic3r/Log.cpp
    ${LIBDIR}/libslic3r/Model.cpp
    ${LIBDIR}/libslic3r/MotionPlanner.cpp
    ${LIBDIR}/libslic3r/MultiPoint.cpp
    ${LIBDIR}/libslic3r/PerimeterGenerator.cpp
    ${LIBDIR}/libslic3r/PlaceholderParser.cpp
    ${LIBDIR}/libslic3r/Point.cpp
    ${LIBDIR}/libslic3r/Polygon.cpp
    ${LIBDIR}/libslic3r/Polyline.cpp
    ${LIBDIR}/libslic3r/PolylineCollection.cpp
    ${LIBDIR}/libslic3r/Print.cpp
    ${LIBDIR}/libslic3r/PrintConfig.cpp
    ${LIBDIR}/libslic3r/PrintObject.cpp
    ${LIBDIR}/libslic3r/PrintRegion.cpp
    ${LIBDIR}/libslic3r/SimplePrint.cpp
    ${LIBDIR}/libslic3r/SlicingAdaptive.cpp
    ${LIBDIR}/libslic3r/Surface.cpp
    ${LIBDIR}/libslic3r/SurfaceCollection.cpp
    ${LIBDIR}/libslic3r/SVG.cpp
    ${LIBDIR}/libslic3r/TriangleMesh.cpp
    ${LIBDIR}/libslic3r/TransformationMatrix.cpp
    ${LIBDIR}/libslic3r/SupportMaterial.cpp
    ${LIBDIR}/libslic3r/utils.cpp
    ${LIBDIR}/libslic3r/miniz_extension.cpp
)
target_compile_features(libslic3r PUBLIC cxx_std_23)
target_include_directories(libslic3r SYSTEM PUBLIC ${SLIC3R_INCLUDES})
target_include_directories(libslic3r PUBLIC ${LIBSLIC3R_INCLUDES})
    
    target_compile_definitions(libslic3r PUBLIC BOOST_NOWIDE_NO_LIB BOOST_NOWIDE_FOUND)

    if(NOT SLIC3R_STATIC)
        target_compile_definitions(libslic3r PUBLIC BOOST_ALL_DYN_LINK)
    endif()

    if(MSVC)
        target_compile_definitions(libslic3r PUBLIC NOMINMAX)
    endif()

add_library(BSpline STATIC
    ${LIBDIR}/BSpline/BSpline.cpp
)
target_include_directories(BSpline PUBLIC ${BSPLINE_INCLUDES})
target_compile_options(BSpline PRIVATE -w)

add_library(admesh STATIC
    ${LIBDIR}/admesh/connect.c
    ${LIBDIR}/admesh/normals.c
    ${LIBDIR}/admesh/shared.c
    ${LIBDIR}/admesh/stl_io.c
    ${LIBDIR}/admesh/stlinit.c
    ${LIBDIR}/admesh/util.c
)
target_include_directories(admesh PUBLIC ${ADMESH_INCLUDES} ${COMMON_INCLUDES})
set_property(TARGET admesh PROPERTY C_STANDARD 99)
target_compile_options(admesh PRIVATE -w)


add_library(clipper STATIC ${LIBDIR}/clipper.cpp)
target_compile_features(clipper PUBLIC cxx_std_23)
target_include_directories(clipper PUBLIC ${COMMON_INCLUDES})
target_compile_options(clipper PRIVATE -w)

add_library(expat STATIC
    ${LIBDIR}/expat/xmlparse.c
    ${LIBDIR}/expat/xmlrole.c
    ${LIBDIR}/expat/xmltok.c
)
target_compile_features(expat PUBLIC cxx_std_23)
target_include_directories(expat PUBLIC ${EXPAT_INCLUDES})
target_compile_options(expat PRIVATE -w)

add_library(polypartition STATIC ${LIBDIR}/polypartition.cpp)
target_include_directories(polypartition PUBLIC ${COMMON_INCLUDES})
target_compile_options(polypartition PRIVATE -w)

add_library(poly2tri STATIC
    ${LIBDIR}/poly2tri/common/shapes.cc
    ${LIBDIR}/poly2tri/sweep/advancing_front.cc
    ${LIBDIR}/poly2tri/sweep/cdt.cc
    ${LIBDIR}/poly2tri/sweep/sweep_context.cc
    ${LIBDIR}/poly2tri/sweep/sweep.cc
)
target_include_directories(poly2tri PUBLIC ${COMMON_INCLUDES})
target_compile_options(poly2tri PRIVATE -w)

if(WIN32)
    add_executable(slic3r WIN32 src/slic3r.cpp package/win/slic3r.rc package/win/slic3r.exe.manifest)
else()
    add_executable(slic3r src/slic3r.cpp)
endif()
target_compile_features(slic3r PUBLIC cxx_std_23)

if (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.10)
cmake_policy(SET CMP0072 NEW)
endif (${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.10)



if(SLIC3R_STATIC)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_STATIC_RUNTIME ON)
else(SLIC3R_STATIC)
    set(Boost_USE_STATIC_LIBS OFF)
    set(Boost_USE_STATIC_RUNTIME OFF)
endif(SLIC3R_STATIC)

find_library(bsystem_l boost_system log)
if(SLIC3R_STATIC)
    add_library(bsystem STATIC IMPORTED)
else(SLIC3R_STATIC)
    add_library(bsystem SHARED IMPORTED)
endif(SLIC3R_STATIC)
set_target_properties(bsystem PROPERTIES IMPORTED_LOCATION ${bsystem_l})
find_library(bthread_l boost_thread)
if(SLIC3R_STATIC)
    add_library(bthread STATIC IMPORTED)
else(SLIC3R_STATIC)
add_library(bthread SHARED IMPORTED)
endif(SLIC3R_STATIC)
set_target_properties(bthread PROPERTIES IMPORTED_LOCATION ${bthread_l})
include_directories(${Boost_INCLUDE_DIRS})

if (Enable_GUI)
if(SLIC3R_STATIC)
    set(wxWidgets_USE_STATIC ON)
else(SLIC3R_STATIC)
    set(wxWidgets_USE_STATIC OFF)
endif(SLIC3R_STATIC)
endif()

if (Enable_GUI)
find_package(OpenGL)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
set(wxWidgets_USE_UNICODE ON)
find_package(wxWidgets COMPONENTS net gl html aui adv core base)
find_package(imgui CONFIG REQUIRED)
endif()

IF(CMAKE_HOST_UNIX)
    #set(Boost_LIBRARIES bsystem bthread bfilesystem)
ENDIF(CMAKE_HOST_UNIX)

# Libraries that Libslic3r itself depends on.
set(LIBSLIC3R_DEPENDS
    admesh
    BSpline
    clipper
    expat
    polypartition
    poly2tri
    ZipArchive
    miniz
    ${Boost_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT} 
)

target_link_libraries(libslic3r PUBLIC ${LIBSLIC3R_DEPENDS})

set(SLIC3R_GUI_INCLUDES
    ${LIBDIR}/libslic3r
    ${GUI_LIBDIR}
    ${OPENGL_INCLUDE_DIR}
    ${wxWidgets_INCLUDE}
)


IF(wxWidgets_FOUND AND OPENGL_FOUND AND Enable_GUI)
    MESSAGE("wx found!")
    MESSAGE("OpenGL found!")
    INCLUDE("${wxWidgets_USE_FILE}")

    add_library(slic3r_gui STATIC 
    ${GUI_LIBDIR}/Dialogs/AboutDialog.cpp
    ${GUI_LIBDIR}/Dialogs/PresetEditor.cpp
    ${GUI_LIBDIR}/Dialogs/PrintEditor.cpp
    ${GUI_LIBDIR}/Dialogs/PrinterEditor.cpp
    ${GUI_LIBDIR}/Dialogs/MaterialEditor.cpp
    ${GUI_LIBDIR}/Dialogs/ObjectCutDialog.cpp
    ${GUI_LIBDIR}/Dialogs/ObjectSettingsDialog.cpp
    ${GUI_LIBDIR}/GUI.cpp
    ${GUI_LIBDIR}/MainFrame.cpp
    ${GUI_LIBDIR}/Plater.cpp
    ${GUI_LIBDIR}/Preset.cpp
    ${GUI_LIBDIR}/Scene3D.cpp

    ${GUI_LIBDIR}/Plater/Plate3D.cpp
    ${GUI_LIBDIR}/Plater/Preview3D.cpp
    ${GUI_LIBDIR}/Plater/PlaterObject.cpp
    ${GUI_LIBDIR}/Plater/PresetChooser.cpp
    ${GUI_LIBDIR}/Plater/ImGuiToolbar.cpp
    ${GUI_LIBDIR}/Settings.cpp
    ${GUI_LIBDIR}/misc_ui.cpp
    ${GUI_LIBDIR}/OptionsGroup.cpp
    ${GUI_LIBDIR}/OptionsGroup/UI_NumChoice.cpp
    ${GUI_LIBDIR}/OptionsGroup/UI_Choice.cpp
    ${GUI_LIBDIR}/OptionsGroup/UI_Point.cpp
    ${GUI_LIBDIR}/OptionsGroup/UI_Point3.cpp
    ${GUI_LIBDIR}/OptionsGroup/UI_Color.cpp
    ${GUI_LIBDIR}/OptionsGroup/UI_Slider.cpp
    ${GUI_LIBDIR}/ExtrusionGeometry.cpp
    ${GUI_LIBDIR}/ConfigWizard.cpp
    ${GUI_LIBDIR}/Preferences.cpp
    ${GUI_LIBDIR}/Theme/ThemeManager.cpp
    ${GUI_LIBDIR}/Theme/CanvasTheme.cpp
    ${GUI_LIBDIR}/Widgets/ThemedControls.cpp
    ${GUI_LIBDIR}/Widgets/SplashScreen.cpp
    ${GUI_LIBDIR}/Widgets/ThemedMenuBar.cpp
    ${GUI_LIBDIR}/Widgets/ThemedMenuPopup.cpp
    ${GUI_LIBDIR}/Widgets/ThemedMenu.cpp
    ${GUI_LIBDIR}/Widgets/PresetWidget.cpp
    ${GUI_LIBDIR}/Dialogs/WidgetGallery.cpp
    ${GUI_LIBDIR}/GL/GLManager.cpp
    ${GUI_LIBDIR}/GL/Shader.cpp
    ${GUI_LIBDIR}/GL/Camera.cpp
    ${GUI_LIBDIR}/GL/Buffer.cpp
    ${GUI_LIBDIR}/ImGuiWrapper.cpp
    )
    target_compile_features(slic3r_gui PUBLIC cxx_std_23)
    target_include_directories(slic3r_gui PUBLIC ${SLIC3R_GUI_INCLUDES} ${SLIC3R_INCLUDES} )
    #only build GUI lib if building with wx
    target_link_libraries (slic3r_gui ${wxWidgets_LIBRARIES} ${OPENGL_LIBRARIES} glad::glad glm::glm imgui::imgui)
    target_link_libraries (slic3r slic3r_gui ${wxWidgets_LIBRARIES} ${OPENGL_LIBRARIES})
    add_compile_options(-DUSE_WX)

    if (WIN32)
        target_link_libraries(slic3r uxtheme)    
    endif()    
ELSE(wxWidgets_FOUND AND OPENGL_FOUND AND Enable_GUI)
    # For convenience. When we cannot continue, inform the user
    if (Enable_GUI)
        if (NOT wxWidgets_FOUND)
            MESSAGE("wx not found!")
        endif (NOT wxWidgets_FOUND)
        if (NOT OPENGL_FOUND)
            MESSAGE("OpenGL not found!")
        endif (NOT OPENGL_FOUND)
    endif (Enable_GUI)
    #skip gui when no wx included
ENDIF(wxWidgets_FOUND AND OPENGL_FOUND AND Enable_GUI)

target_link_libraries (slic3r libslic3r ${LIBSLIC3R_DEPENDS})

if (BUILD_EXTRUDE_TIN)
    add_executable(extrude-tin src/utils/extrude-tin.cpp)
    set_target_properties(extrude-tin PROPERTIES LINK_SEARCH_START_STATIC 1)
    set_target_properties(extrude-tin PROPERTIES LINK_SEARCH_END_STATIC 1)
    target_link_libraries (extrude-tin libslic3r ${LIBSLIC3R_DEPENDS})
endif() 

# Windows needs a compiled component for Boost.nowide
IF (WIN32)
    if (NOT BOOST_NOWIDE_FOUND)
        if(SLIC3R_STATIC)
            set(BOOST_NOWIDE_LIB_TYPE STATIC)
        else()
            set(BOOST_NOWIDE_LIB_TYPE SHARED)
        endif()

        add_library(boost-nowide ${BOOST_NOWIDE_LIB_TYPE} 
            ${LIBDIR}/boost_bundled/nowide/iostream.cpp
        )
        if(NOT SLIC3R_STATIC)
            target_compile_definitions(boost-nowide PRIVATE BOOST_NOWIDE_SOURCE)
        endif()

        if(MSVC)
            target_compile_definitions(boost-nowide PUBLIC BOOST_NOWIDE_NO_LIB)
        endif()    
        target_include_directories(boost-nowide PRIVATE ${LIBDIR}/boost_bundled)
        target_link_libraries(slic3r boost-nowide)
        # Link to libslic3r so tests inherit it too
        target_link_libraries(libslic3r PUBLIC boost-nowide)
        target_include_directories(boost-nowide PUBLIC ${COMMON_INCLUDES})
    endif()

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
       set(CMAKE_EXE_LINKER_FLAGS "-Wl,--allow-multiple-definition")
    endif()
ENDIF(WIN32)

# Auto-set the startup project in Visual Studio
if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT slic3r)
    
    # Copy resources to the Debug/Release folder so running from VS works
    add_custom_command(TARGET slic3r POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:slic3r>/resources"
        COMMENT "Copying resources to output directory..."
    )
endif()