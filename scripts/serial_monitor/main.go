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
var logFile string
var clearMode bool

func main() {
	flag.StringVar(&addr, "a", ":6666", "")
	flag.StringVar(&logFile, "f", "last.log", "")
	flag.BoolVar(&clearMode, "c", true, "clear log file every time")
	flag.Parse()

	// 创建一个可写的文本文件
	f, err := os.Create(logFile)
	if err != nil {
		log.Fatal(err)
	}
	defer f.Close()

	for {
		var modeStr string
		if clearMode {
			modeStr = "clear log file every time"
		} else {
			modeStr = "do not clear log file"
		}
		log.Printf("listen addr %v log file %v %v", addr, logFile, modeStr)

		monitor(f)


	}
}

func monitor(f *os.File) {
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
			fmt.Printf("\r\033[Kwait %d seconds", int(time.Since(start).Seconds()))
			time.Sleep(500 * time.Millisecond)
		}
	}

	defer conn.Close()



	// 每次连接成功，就清空这个文件，从头开始写数据
	if clearMode {
		err = f.Truncate(0)
		if err != nil {
			log.Fatal(err)
		}

		f.Seek(0, 0)
	}

	// 写入当前时间
	f.WriteString("\n\n")
	_, err = f.WriteString(time.Now().Format("2006-01-02 15:04:05.000") + "\n")
	if err != nil {
		log.Fatal(err)
	}
	f.WriteString("\n\n")

	end := make(chan bool, 0)

	// 将串行控制台的输出发送到屏幕
	go func() {
		// 将conn的输出复制到os.Stdout和f
		_, err = io.Copy(io.MultiWriter(os.Stdout, f), conn)
		if err != nil {
			log.Fatal(err)
		}
		fmt.Printf("qemu -> stdout end\n")
		end <- true
	}()

	// 将用户输入发送到串行控制台
	go func() {
		_, _ = io.Copy(conn, os.Stdin)
		log.Printf("stdin -> qemu end\n")
		end <- true
	}()

	// 只要有一方断开就退出
	<-end
}
