# Core/Misc subpackage

This subpackage includes utility functions that are used internally by other parts of Core package but are rarely recommended to use directly. It includes:

1. Raw memory copying and comparison with unsafe pointer interface. Prefer using Core/Range subpackage which uses them to achieve maximum performance for trivial types.
2. Fast integer and floating point to string conversion. Prefer using Core/Range/Stream subpackage which wraps them in a safer interface.
3. Some integer arithmetic tricks for faster division and multiplication. Don't use unless you really need low-level optimizations to speed up your code.

