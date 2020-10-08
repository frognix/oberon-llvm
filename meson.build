project('thesis-test', 'cpp',
        default_options: ['cpp_std=c++2a'])

incdir = include_directories('include')

src = ['./src/main.cpp', './src/reassign_ptr.cpp']

fmt_dep = dependency('fmt')

executable('thesis-test',
           sources: src,
           include_directories: incdir,
           dependencies : [fmt_dep],
           override_options : ['cpp_std=c++2a'],
           c_args: ['-Wall', '-fanalyzer', '-Werror'])
