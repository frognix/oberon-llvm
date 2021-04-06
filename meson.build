project('oberon-llvm', 'cpp')

opt = ['cpp_std=c++2a', 'warning_level=3']

# add_project_arguments('-pg', language : 'cpp')

incdir = include_directories('include')

src = [
  './src/format.cc', #fmtlib : https://github.com/fmtlib/fmt
  './src/main.cpp',
  './src/parser.cpp',
  './src/expression_nodes.cpp',
  './src/type_nodes.cpp',
  './src/symbol_container.cpp',
  './src/procedure_table.cpp',
  './src/simple_procedure_table.cpp',
  './src/module_table.cpp',
  './src/statement_nodes.cpp',
  './src/section_nodes.cpp',
  './src/module_loader.cpp',
  './src/io_manager.cpp',
  './src/file_manager.cpp',
  './src/message_container.cpp'
]

# fmt_dep = dependency('fmt')

executable('oberon-llvm',
           sources: src,
           include_directories: incdir,
           # dependencies : [fmt_dep],
           override_options : opt)
