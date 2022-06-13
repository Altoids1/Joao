project('joao', 'cpp',
		default_options : ['cpp_std=c++17', 'warning_level=0'],
		license : 'MIT')

VERSION = '2.1.0'

src = [
	'Args.cpp',
	'AST.cpp',
	'BinaryOperations.cpp',
	'Directory.cpp',
	'Interpreter.cpp',
	'main.cpp',
	'Object.cpp',
	'Parser.cpp',
	'Program.cpp',
	'Scanner.cpp',
	'Table.cpp',
	'Value.cpp',
]

nativefuncs = [
	'nativefuncs/error.cpp',
	'nativefuncs/file.cpp',
	'nativefuncs/math.cpp',
	'nativefuncs/string.cpp',
	'nativefuncs/tablelib.cpp',
]

if build_machine.system() == 'windows'
	windows = import('windows')
	src += windows.compile_resources('./Joao.rc')
endif

version_parts = VERSION.split('.')
configure_file(input : 'config.h.in', output : 'config.h',
  configuration : {
    'version': VERSION,
	'major': version_parts[0],
	'minor': version_parts[1],
	'patch': version_parts[2],
  }
)

exe = executable('joao', nativefuncs + src)

# Diagnostic tests
test('hello world', exe, args : ['./testprograms/tests/1_helloworld.jao'], workdir : meson.source_root(), suite: 'diagnostic')
test('blocks', exe, args : ['./testprograms/tests/2_blocks.jao'], workdir : meson.source_root(), suite: 'diagnostic')
test('functions', exe, args : ['./testprograms/tests/3_functions.jao'], workdir : meson.source_root(), suite: 'diagnostic')
test('objects', exe, args : ['./testprograms/tests/4_objects.jao'], workdir : meson.source_root(), suite: 'diagnostic')
test('strings', exe, args : ['./testprograms/tests/5_strings.jao'], workdir : meson.source_root(), suite: 'diagnostic')
test('parser', exe, args : ['./testprograms/tests/6_parser.jao'], workdir : meson.source_root(), suite: 'diagnostic')
test('rvalues', exe, args : ['./testprograms/tests/7_rvalues.jao'], workdir : meson.source_root(), suite: 'diagnostic')
test('tablelib', exe, args : ['./testprograms/tests/8_tablelib.jao'], workdir : meson.source_root(), suite: 'diagnostic')

# Benchmark tests
benchmark('primegen', exe, args : ['./testprograms/bench/primegen.jao'], workdir : meson.source_root())
benchmark('ackermann', exe, args : ['./testprograms/bench/ackermann.jao'], workdir : meson.source_root())
benchmark('jsoninterp', exe, args : ['./testprograms/bench/json_interp.jao'], workdir : meson.source_root())
