# Per-project defaults for font-crusher.
#
# NOTE every preset here resolves to ${sourceDir}/build -- none override binaryDir -- so the
# collision guard in light-configure.ps1 is the only thing standing between a debug tree and an
# address-sanitizer one. build-addrsan on disk was configured by hand for that reason.
#
# crush.exe lands in build/bin/, not build/module/crush/, because CMakeLists.txt sets
# CMAKE_RUNTIME_OUTPUT_DIRECTORY to ${CMAKE_BINARY_DIR}/bin so the DLLs sit beside it.
@{
        Name = 'font-crusher'

        Trees = @{
                'conf-crush-debug'           = 'build'
                'conf-crush-debug-nocontext' = 'build'
                'conf-crush-trace'           = 'build'
                'conf-crush-trace-nocontext' = 'build'
                'conf-crush-release'         = 'build'
                # hand-configured: conf-crush-debug-addrsan writes to build/, not build-addrsan/
                'conf-crush-debug-addrsan'   = 'build-addrsan'
        }

        Expect = @{
                'conf-crush-debug'         = @{ LIGHT_PLATFORM = 'HOST'; CMAKE_BUILD_TYPE = 'Debug' }
                'conf-crush-release'       = @{ LIGHT_PLATFORM = 'HOST'; CMAKE_BUILD_TYPE = 'Release' }
                'conf-crush-debug-addrsan' = @{ LIGHT_PLATFORM = 'HOST' }
        }

        Targets = @{
                'crush' = @{ Preset = 'conf-crush-debug' }
        }

        DefaultTarget = 'crush'

        #   crush is the heaviest consumer of light_cli -- 23 commands exercising registration,
        # parsing, dispatch and aliasing, plus the full module load/unload cycle -- and none of
        # that is under CTest. `crush font list` exercising all of it and exiting 0 is a
        # meaningful check that a bare test count would miss.
        Test = @{
                Preset = 'conf-crush-debug'
                Target = 'crush'
                Ctest  = $true
                #   no .exe: light-test.ps1 appends the platform's suffix, so this one entry is
                # correct on Windows and on the Linux runners CI uses. Spelled with .exe it was
                # a "not built" failure everywhere except Windows.
                Smoke  = @(
                        @{ Exe = 'bin/crush'; Args = @('font', 'list') }
                )
        }

        #   everything lands in bin/ here, because CMakeLists sets CMAKE_RUNTIME_OUTPUT_DIRECTORY
        # so the DLLs sit beside crush. Nearly all of this suite's coverage comes from the crush
        # binary itself: the tests are ctest invocations of crush subcommands rather than unit
        # tests linked against the library.
        #   the vendored freetype and jansson trees are excluded -- measuring third-party code
        # we do not test would only dilute the number that matters
        Coverage = @{
                Objects     = 'bin/*'
                IgnoreRegex = '(/lib/|/usr/|sanitizers/|_deps/|/freetype/|/jansson/)'
                CMakeArgs   = @('-DCOPY_CONTEXT=true', '-DCOPY_CONTEXT_NAME=test_context_default')
        }
}
