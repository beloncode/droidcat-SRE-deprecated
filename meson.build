project('droidcat', ['c'], version: '000a0')
root_src = files('main_thread.c')
compiler_args = [
    '-std=c11', 
    '-fstack-protector', 
    '-O2', 
    '-ffast-math', 
    '-Wall', 
    '-Werror',
    '-Wno-error=stringop-truncation'
]
executable(meson.project_name(), sources: [root_src], c_args: compiler_args)

