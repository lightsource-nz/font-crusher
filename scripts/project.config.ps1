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
                Smoke  = @(
                        @{ Exe = 'bin/crush.exe'; Args = @('font', 'list') }
                )
        }
}
