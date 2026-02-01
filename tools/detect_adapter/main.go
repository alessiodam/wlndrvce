package main

import (
	"bufio"
	"fmt"
	"net/http"
	"strings"
	"time"

	"github.com/google/gousb"
)

func main() {
	db := make(map[string]string)
	resp, _ := http.Get("http://www.linux-usb.org/usb.ids")
	scanner := bufio.NewScanner(resp.Body)
	var curV string
	for scanner.Scan() {
		line := scanner.Text()
		if line == "" || strings.HasPrefix(line, "#") {
			continue
		}
		if !strings.HasPrefix(line, "\t") {
			curV = strings.Fields(line)[0]
		} else if curV != "" {
			f := strings.SplitN(strings.TrimSpace(line), "  ", 2)
			if len(f) == 2 {
				db[curV+":"+f[0]] = f[1]
			}
		}
	}
	resp.Body.Close()
	ctx := gousb.NewContext()
	defer ctx.Close()
	seen := make(map[string]bool)
	fmt.Println("Monitoring USB ports...")
	for {
		devs, _ := ctx.OpenDevices(func(d *gousb.DeviceDesc) bool { return true })
		current := make(map[string]bool)
		for _, d := range devs {
			id := fmt.Sprintf("%04x:%04x", int(d.Desc.Vendor), int(d.Desc.Product))
			current[id] = true
			if !seen[id] {
				name := strings.ToLower(db[id])
				fmt.Printf("[+] Plugged: %s (%s)\n", id, name)
				seen[id] = true
			}
			d.Close()
		}
		for k := range seen {
			if !current[k] {
				delete(seen, k)
				fmt.Println("[-] Unplugged:", k)
			}
		}
		time.Sleep(time.Second)
	}
}
