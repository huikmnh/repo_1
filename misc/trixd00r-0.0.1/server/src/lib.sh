gcc -O2 -c -fPIC -fvisibility=hidden -I ../inc checks.c  controller.c  daemon.c  error.c  help.c  lib.c  shell.c  signals.c  sniffer.c  wrapper.c
ld -Bshareable -o libttt.so checks.o  controller.o  daemon.o  error.o  help.o  lib.o  shell.o  signals.o  sniffer.o    wrapper.o pcap.a -lpthread -lc
cp libttt.so blob.bin
objcopy -I binary -O elf64-x86-64 -B i386:x86-64 blob.bin blob.o
