# Future regression suite

No new test code is created during this foundation pass, following the explicit
instruction under REQUIRED TEST CASES in the master brief. See docs/TESTING.md
for existing upstream validation results and the pending regression matrix.

No physical BLE backend may be linked into automated tests. Shared simulator mocks
live in simulator/ and should be reused rather than copied here.
