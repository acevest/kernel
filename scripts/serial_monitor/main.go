package main

import (
	"flag"
	"fmt"
	"io"
	"log"
	"net"
	"os"
	"strings"
	"time"
)

var addr string
var logFile string
var clearMode bool

func main() {
	flag.StringVar(&addr, "a", ":6666", "")
	flag.StringVar(&logFile, "f", "last.log", "")
	flag.BoolVar(&clearMode, "c", true, "clear log file and screen every time")
	flag.Parse()

	// 创建一个可写的文本文件
	f, err := os.Create(logFile)
	if err != nil {
		log.Fatal(err)
	}
	defer f.Close()

	for {
		monitor(f)
	}
}

func monitor(f *os.File) {
	var conn net.Conn
	var err error

	start := time.Now()


	var modeStr string
	if clearMode {
		modeStr = "clear log file every time"
	} else {
		modeStr = "do not clear log file"
	}
	log.Printf("lissten addr %v log file %v %v", addr, logFile, modeStr)


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


	if clearMode {
		err = f.Truncate(0)
		if err != nil {
			log.Fatal(err)
		}

		f.Seek(0, 0)

		// 组合转义码：清屏 + 清除滚动缓冲区
		// \033[H  光标移到左上角
		// \033[2J 清空屏幕
		// \033[3J  清除滚动缓冲区（部分终端支持）
		fmt.Print("\033[H\033[2J\033[3J")

		//
		var divideLine = strings.Repeat("-", 80)
		fmt.Printf("\n%v\n\nSCREEN CLEARED AT %v\n\n%v\n\n", divideLine, time.Now().Format("2006-01-02 15:04:05.000"), divideLine)
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
