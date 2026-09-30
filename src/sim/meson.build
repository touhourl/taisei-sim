taisei_sim = shared_library(
    'taisei_sim',
    files('sim.c', 'runtime.c'),
    dependencies : libtaisei_dep.as_link_whole(),
    c_args : [taisei_c_args, '-DTAISEI_SIM_BUILD'],
    gnu_symbol_visibility : 'hidden',
    link_args : taisei_link_args,
    install : false,
    build_rpath : '$ORIGIN',
    install_rpath : '$ORIGIN',
)
