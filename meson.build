project('droidcat', ['c'], version: '000a0', default_options: [])
root_src = files(
    'Main_Thread.c',
    'Thread_Pool.c', 
)
data_src = files(
    'data/Doubly_Linked.c',
    'data/FIFO_Queue.c'
)
cpu_src = files(
    'cpu/CPU_Time.c',
    'cpu/Hardware_Info.c',
    'cpu/Physical_CPU.c'
)

compiler_args = [
    '-std=c11', 
    '-fstack-protector',
    '-ffast-math', 
    '-Wall', 
    '-Werror'
]
thread_dep = dependency('threads')
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
    compiler_args += '-O1'
endif

executable(meson.project_name(), sources: [root_src, data_src, cpu_src], c_args: compiler_args, dependencies: thread_dep)

tpool_test_src = files('unit/Thread_Pool_TEST.c', 'Thread_Pool.c')
tpool_test = executable('thread_pool_test', sources: [tpool_test_src, data_src, cpu_src], dependencies: thread_dep)
test('Unit Thread Pool Test', tpool_test)

doubly_test_src = files('unit/Doubly_Linked_TEST.c')
doubly_test = executable('doubly_test', sources: [doubly_test_src, data_src], dependencies: thread_dep)
test('Doubly Linked List Test', doubly_test)
