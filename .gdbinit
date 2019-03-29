set sysroot /home/shua/tools_workspace/dropb/buildroot/output/staging
target extended-remote 10.0.0.87:2345
remote put main /usr/bin/main
set remote exec-file main
break main