set sysroot /home/shua/tools_workspace/dropb/buildroot/output/staging
target extended-remote 10.0.0.87:2345
remote put test_temp /usr/bin/test_temp
set remote exec-file test_temp
break main