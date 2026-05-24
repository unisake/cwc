project('cwc', 'c',
    version: '0.1.0',
    default_options: ['c_std=c23']
)

wlroots = dependency('wlroots-0.21')
wayland_server = dependency('wayland-server')
xkbcommon = dependency('xkbcommon')

executable('cwc',
    sources: [
        'src/core.c',
        'src/wm.c',
        'src/km.c',
        'src/pm.c',
    ],
    dependencies: [
        wlroots,
        wayland_server,
        xkbcommon,
    ],
    include_directories: include_directories('include'),
    c_args: ['-DWLR_USE_UNSTABLE', '-D_POSIX_C_SOURCE=200809L'],
)