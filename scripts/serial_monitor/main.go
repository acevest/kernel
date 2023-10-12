package main

import (
	"flag"
	"fmt"
	"io"
	"log"
	"net"
	"os"
	"time"
)

var addr string

func main() {
	flag.StringVar(&addr, "a", ":6666", "")
	flag.Parse()

	for {
		log.Printf("addr %v", addr)
		monitor()
	}
}

func monitor() {
	var conn net.Conn
	var err error

	start := time.Now()

	// 不断尝试连接串行控制台
	for {
		conn, err = net.Dial("tcp", addr)
		if err == nil {
			log.Printf("\nconnected...\n")
			break
		} else {
			fmt.Printf("\r\033[Kwait %d seconds", int(time.Now().Sub(start).Seconds()))
			time.Sleep(500 * time.Millisecond)
		}
	}

	// // 创建一个 WaitGroup 以便在两个协程完成时结束程序
	// var wg sync.WaitGroup
	// wg.Add(2)

	// 将串行控制台的输出发送到屏幕
	go func() {
		_, _ = io.Copy(os.Stdout, conn)
		// wg.Done()
	}()

	// 将用户输入发送到串行控制台
	go func() {
		_, _ = io.Copy(conn, os.Stdin)
		// wg.Done()
	}()

	// // 等待两个协程完成
	// wg.Wait()
}
