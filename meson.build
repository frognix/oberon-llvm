project('thesis-test', 'cpp')

opt = ['cpp_std=c++2a', 'warning_level=3']

# add_project_arguments('-pg', language : 'cpp')

incdir = include_directories('include')

src = [
  './src/main.cpp',
  './src/format.cpp',
  './src/reassign_core.cpp',
  './src/code_stream.cpp',
  './src/parser.cpp'
]

fmt_dep = dependency('fmt')

executable('thesis-test',
           sources: src,
           include_directories: incdir,
           dependencies : [fmt_dep],
           override_options : opt)
