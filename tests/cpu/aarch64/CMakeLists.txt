qbox_add_cpu_test(aarch64-simple-write-test 100 simple-write-test.cc)
qbox_add_cpu_test(aarch64-dmi-test 100 dmi-test.cc)
qbox_add_cpu_test(aarch64-dmi-test-concurrent-inval 100 dmi-test-concurrent-inval.cc)
qbox_add_cpu_test(aarch64-ld-st-excl-fail-test 100 ld-st-excl-fail.cc)
qbox_add_cpu_test(aarch64-write_read 100 write_read.cc)

#This test will fail for now
qbox_add_cpu_test(aarch64-dmi-test-async-inval 100 dmi-test-async-inval.cc)