package main

import (
	"encoding/binary"
	"flag"
	"fmt"
	"io"
	"log"
	"os"
	"path/filepath"
	"strings"
	"time"
)

const MagicCode = 0x66179BF8

type FileHeader struct {
	Magic        uint32
	HeaderSize   uint32
	Timestamp    uint32
	FileEntryCnt uint32
	Name         string
}

func (f *FileHeader) CalculateHeaderSize() {
	f.HeaderSize = uint32(4 + 4 + 4 + 4 + len(f.Name))
}

type FileType uint32

const (
	FileTypeUnknown FileType = iota
	FileTypeNormal
	FileTypeDir
	FileTypeSymlink
)

func (t FileType) String() string {
	s := ""
	switch t {
	case FileTypeNormal:
		s += "normal"
	case FileTypeDir:
		s += "dir"
	case FileTypeSymlink:
		s += "symlink"
	default:
		s += "unknown"
	}
	s = fmt.Sprintf("%10v:%v", s, uint32(t))
	return s
}

type FileEntry struct {
	EntrySize uint32
	FileType
	FileName string // 生成在initrd文件中的文件名
	FileSize uint32
	Offset   uint32 // 在文件中的偏移
	RealPath string // 这个不写到输出文件中
}

func (e FileEntry) String() string {
	s := fmt.Sprintf("%v:%v:%v", e.EntrySize, e.FileType, e.FileName)
	return s
}

func main() {

	var inputPath string
	var outputFileName string

	flag.StringVar(&inputPath, "path", "", "Path to the folder to be packed")
	flag.StringVar(&outputFileName, "name", "", "Name of the generated file")

	flag.Parse()

	if inputPath == "" || outputFileName == "" {
		flag.Usage()
		return
	}

	// 获取当前目录
	currentDir, err := os.Getwd()
	if err != nil {
		log.Fatal(err)
	}

	// 计算相对路径
	relPath, err := filepath.Rel(currentDir, filepath.Join(currentDir, inputPath))
	if err != nil {
		log.Fatal(err)
	}

	pathPrefix := relPath

	var entries []FileEntry

	err = filepath.Walk(pathPrefix, func(filePath string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}

		fileType := FileTypeUnknown
		fileSize := uint32(0)
		switch {
		case info.Mode().IsRegular():
			fileType = FileTypeNormal
			fileInfo, err := os.Stat(filePath)
			if err != nil {
				log.Fatal(err)
			}
			fileSize = uint32(fileInfo.Size())
		case info.Mode().IsDir():
			fileType = FileTypeDir
		case info.Mode()&os.ModeSymlink != 0:
			fileType = FileTypeSymlink
			log.Fatalf("unsupport symlink file")
		default:
			log.Fatalf("unsupport file type for %v", filePath)
		}

		if filePath == pathPrefix {
			return nil
		}

		path := strings.TrimPrefix(filePath, pathPrefix)

		entry := FileEntry{
			EntrySize: uint32(8 + len(path)),
			FileType:  fileType,
			FileName:  path,
			FileSize:  fileSize,
			RealPath:  filePath,
		}
		entries = append(entries, entry)

		fmt.Printf("%v\n", entry)

		return nil
	})

	if err != nil {
		log.Fatal(err)
	}

	outputFile, err := os.Create(outputFileName)
	if err != nil {
		log.Fatal(err)
	}
	defer outputFile.Close()

	// 写入文件头
	var hdr FileHeader
	hdr.Magic = MagicCode
	hdr.HeaderSize = 0
	hdr.Timestamp = uint32(time.Now().Unix())
	hdr.FileEntryCnt = uint32(len(entries))
	hdr.Name = outputFileName + "\x00"
	hdr.CalculateHeaderSize()

	binary.Write(outputFile, binary.LittleEndian, hdr.Magic)
	if err != nil {
		log.Fatal(err)
	}
	binary.Write(outputFile, binary.LittleEndian, hdr.HeaderSize)
	if err != nil {
		log.Fatal(err)
	}
	binary.Write(outputFile, binary.LittleEndian, hdr.Timestamp)
	if err != nil {
		log.Fatal(err)
	}
	binary.Write(outputFile, binary.LittleEndian, hdr.FileEntryCnt)
	if err != nil {
		log.Fatal(err)
	}
	_, err = io.WriteString(outputFile, hdr.Name)
	if err != nil {
		log.Fatal(err)
	}

	// // 写入文件项数量
	// err = binary.Write(outputFile, binary.LittleEndian, uint32(len(entries)))
	// if err != nil {
	// 	log.Fatal(err)
	// }

	// 写入每个文件项
	currentOffset := uint32(4 + len(entries)*12)
	for _, entry := range entries {
		entry.Offset = currentOffset

		err = binary.Write(outputFile, binary.LittleEndian, entry.EntrySize)
		if err != nil {
			log.Fatal(err)
		}

		err = binary.Write(outputFile, binary.LittleEndian, entry.FileType)
		if err != nil {
			log.Fatal(err)
		}

		_, err = outputFile.WriteString(entry.FileName)
		if err != nil {
			log.Fatal(err)
		}

		currentOffset += entry.FileSize
	}

	// 逐个读取文件并追加到 outputFileName
	for _, entry := range entries {
		if entry.FileType == 1 {
			file, err := os.Open(entry.RealPath)
			if err != nil {
				log.Fatal(err)
			}
			defer file.Close()

			_, err = io.Copy(outputFile, file)
			if err != nil {
				log.Fatal(err)
			}
		}
	}

	fmt.Printf("Binary file '%v' created.\n", outputFileName)
}
