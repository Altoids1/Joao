project('joao', 'cpp',
		default_options : ['cpp_std=c++17', 'warning_level=0'],
		license : 'MIT')

VERSION = '2.3.0'

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
	'Terminal.cpp',
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
if build_machine.system() == 'linux'
	src += 'Daemon.cpp'
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
if build_machine.system() == 'emscripten'
	exe =  executable('joao', nativefuncs + src,
		cpp_args : [],
		link_args : ['-sASYNCIFY'],
		cpp_pch : 'pch/PrecompiledHeaders.h'
	)
else
	exe = executable('joao', nativefuncs + src, cpp_pch : 'pch/PrecompiledHeaders.h')
endif

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
	'constexpr',
	'to_json',
]
foreach diag : diagnostics
	str = './testprograms/tests/mustpass/@0@.jao'.format(diag)
	test(diag, exe, args: [str], workdir : meson.source_root(), suite: 'diagnostic', timeout : 2)
endforeach

## Tests that should fail
shouldfail = [
	'nomain',
	'toobig',
	'file_doesnt_exist', #Phantom test :^)
]
foreach diag : shouldfail
	str = './testprograms/tests/shouldfail/@0@.jao'.format(diag)
	test(diag, exe, args: [str], workdir : meson.source_root(), suite: 'diagnostic', should_fail : true, timeout : 2)
endforeach

## Benchmark tests
benchmarks = [
	'ackermann',
	'bubblesort',
	'json_interp',
	'primegen',
]
foreach diag : benchmarks
	str = './testprograms/bench/@0@.jao'.format(diag)
	benchmark(diag, exe, args: [str], workdir : meson.source_root(), timeout : 10)
endforeach
