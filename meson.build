project('droidcat', ['c'], version: '000a0', default_options: [])
root_src = files('Main_Thread_Start.c')

compiler_args = [
    '-std=c11', 
    '-fstack-protector',  
    '-ffast-math', 
    '-Wall', 
    '-Werror'
]

c_id = meson.get_compiler('c')

host_compiler = c_id.get_id()

if get_option('enable_debug')
    compiler_args += '-fno-optimize-sibling-calls'
    compiler_args += '-fno-omit-frame-pointer'
    compiler_args += '-ggdb'
    compiler_args += '-O1'
    if host_compiler == 'clang'
        compiler_args += '-fsanitize=address'
    endif
else
    compiler_args += '-O2'
endif

message(compiler_args)

executable(meson.project_name(), sources: [root_src], c_args: compiler_args )

