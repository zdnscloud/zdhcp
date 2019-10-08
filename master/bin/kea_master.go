package main

import (
	"flag"

	"kea"
	"kea/command"
)

var (
	ip       string
	port     int
	confFile string
)

func init() {
	flag.StringVar(&ip, "i", "127.0.0.1", "service ip")
	flag.IntVar(&port, "p", 6001, "command service port")
	flag.StringVar(&confFile, "c", "kea.conf", "config file")
}

func main() {
	flag.Parse()
	server, err := kea.NewServer(confFile)
	if err != nil {
		panic(err.Error())
	}

	command.RunTcpCmdServer(server, &command.EndPoint{
		IP:   ip,
		Port: port,
	})
}
