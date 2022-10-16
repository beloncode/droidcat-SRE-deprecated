project('droidcat', ['c'], version: '000a0')
root_src = files('Main_Thread_Start.c')
compiler_args = [
    '-std=c11', 
    '-fstack-protector', 
    '-O2', 
    '-ffast-math', 
    '-Wall', 
    '-Werror',
    '-ggdb'
]
executable(meson.project_name(), sources: [root_src], cpp_args: compiler_args)

