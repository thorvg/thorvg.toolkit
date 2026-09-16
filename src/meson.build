thorvg_dep = dependency('thorvg-1', version: '>=1.0.0')
sdl_dep = dependency('sdl3')
toolkit_dep = [thorvg_dep, sdl_dep]
compiler_flags = []

lib_type = get_option('default_library')

if lib_type == 'shared'
    compiler_flags += ['-DTVG_TOOLKIT_EXPORT', '-DTVG_TOOLKIT_BUILD']
else
    compiler_flags += ['-DTVG_TOOLKIT_STATIC']
endif

# GL or GLES
gl_dep = dependency('gl', required: false)
gles_dep = dependency('glesv2', required: false)

if gl_dep.found()
    toolkit_dep += gl_dep
elif gles_dep.found()
    toolkit_dep += gles_dep
    compiler_flags += ['-DTVG_GLES_SUPPORTED']
else
    message('Neither OpenGL nor OpenGL ES was found.')
endif

# WebGPU
wgpu_dep = dependency('wgpu_native', required: false)

if wgpu_dep.found()
    toolkit_dep += wgpu_dep
    compiler_flags += ['-DTVG_WGPU_SUPPORTED']
    if host_machine.system() == 'darwin'
        add_languages('objcpp', native: false)
        toolkit_dep += declare_dependency(link_args: ['-lobjc', '-framework', 'Cocoa', '-framework', 'CoreVideo', '-framework', 'IOKit', '-framework', 'QuartzCore'])
        compiler_flags += ['-x', 'objective-c++']
    endif
else
    message('wgpu_native not found.')
endif

thorvg_toolkit_lib = library(
    'thorvg-toolkit',
    'tvgToolkit.cpp',
    'tvgWindow.cpp',
    include_directories : thorvg_toolkit_inc,
    version             : meson.project_version(),
    soversion           : host_machine.system() != 'windows' ? meson.project_version() : '',
    dependencies        : toolkit_dep,
    cpp_args            : compiler_flags,
    install             : true
)

thorvg_toolkit_dep = declare_dependency(
    include_directories: thorvg_toolkit_inc,
    dependencies: thorvg_dep,
    link_with: thorvg_toolkit_lib,
    compile_args: (lib_type == 'static') ? ['-DTVG_TOOLKIT_STATIC'] : [],
)
meson.override_dependency('thorvg-toolkit', thorvg_toolkit_dep)

pkg_mod = import('pkgconfig')

pkg_mod.generate(
    libraries        : thorvg_toolkit_lib,
    version          : meson.project_version(),
    name             : 'libthorvg-toolkit',
    filebase         : 'thorvg-toolkit',
    description      : 'ThorVG Application Toolkit',
    requires         : thorvg_dep
)
