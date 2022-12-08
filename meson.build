project('joao', 'cpp',
		default_options : ['cpp_std=c++17', 'warning_level=0'],
		license : 'MIT')

VERSION = '2.1.1'

src = [
	'Args.cpp',
	'AST.cpp',
	'BinaryOperations.cpp',
	'Directory.cpp',
	'FailureOr.cpp',
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
diagnostics = [ #Meson has a philosophical disagreement with procedurally generating the list of files in a directory, so...
	'helloworld',
	'blocks',
	'functions',
	'objects',
	'strings',
	'parser',
	'rvalues',
	'tablelib',
	'json_interp',
	'arithmetic',
	'comparison',
	'mathlib',
	'stdlib',
	'stringlib',
	'indirect_access',
	'try_catch',
	'break',
]
foreach diag : diagnostics
	str = './testprograms/tests/mustpass/@0@.jao'.format(diag)
	test(diag, exe, args: [str], workdir : meson.source_root(), suite: 'diagnostic', timeout : 2)
endforeach

## Tests that should fail
shouldfail = [
	'nomain',
	'file_doesnt_exist', #Phantom test :^)
]
foreach diag : shouldfail
	str = './testprograms/tests/shouldfail/@0@.jao'.format(diag)
	test(diag, exe, args: [str], workdir : meson.source_root(), suite: 'diagnostic', should_fail : true, timeout : 2)
endforeach




# Benchmark tests
benchmark('primegen', exe, args : ['./testprograms/bench/primegen.jao'], workdir : meson.source_root())
benchmark('ackermann', exe, args : ['./testprograms/bench/ackermann.jao'], workdir : meson.source_root())
benchmark('jsoninterp_b', exe, args : ['./testprograms/bench/json_interp.jao'], workdir : meson.source_root())
