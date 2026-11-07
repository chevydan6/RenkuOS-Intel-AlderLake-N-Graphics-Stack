# Qwen VM Validation Report

## Objective
Validate existing GPU regression tests (PPGTT and PPGTT tree) inside renkuos-vm with native GCC 13.3.0.

## Environment
- VM: `renkuos-tests` (Haiku hrev60072+518)
- Compiler: GCC 13.3.0 (native to VM)
- Test Directory: `/boot/home/test-work/qwen-ppgtt-001`
- Validation Scope: Offline host validation only, no GPU access

## Test Results

### 1. n100-ppgtt-test.cpp
- **Compilation**: ✅ Success with flags `-std=c++17 -O2 -Wall -Wextra -Werror -I.`
- **Execution**: ✅ Success
- **Output**: `2150 PPGTT encoding/range cases passed; offline only, no GPU access.`
- **Binary Size**: 8,120 bytes

### 2. n100-ppgtt-tree-test.cpp
- **Compilation**: ✅ Success with flags `-std=c++17 -O2 -Wall -Wextra -Werror -I.`
- **Execution**: ✅ Success
- **Output**: `69 owned offline PPGTT construction cases passed; no GPU access.`
- **Binary Size**: 23,480 bytes

## Verification Details

All tests were compiled and executed in the VM environment with:
- Native GCC 13.3.0 compiler
- C++17 standard
- Optimization level O2
- All warnings treated as errors (-Werror)
- Required include paths specified (-I.)

## Important Notes

- These validations are **offline host tests only** - they do not establish GPU execution or native module correctness
- No hardware access or physical N100 operations were performed
- Test results confirm the code compiles and runs correctly in the VM environment
- The VM environment matches the physical baseline kernel/SDK hashes
- These results are distinct from physical GPU evidence and should be treated accordingly

## Guest Log Directory
`/boot/home/test-work/qwen-ppgtt-001/`

## Observed Counts
- PPGTT encoding/range cases: 2150 passed
- PPGTT construction cases: 69 passed

This VM validation confirms that the existing test infrastructure functions correctly in the native Haiku environment without requiring physical hardware access.
