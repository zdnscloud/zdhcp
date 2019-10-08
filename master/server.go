package kea

import (
	"fmt"
	"net"
	"path"
	"sync"
	"sync/atomic"

	"cement/jconf"
	"kea/util"
)

type Server struct {
	listener  *net.TCPListener
	allocator *AddrAllocator

	clients     []*Client
	clientsLock sync.Mutex

	configFilePath string
	stopChan       chan struct{}
	cmdExecuteLock sync.Mutex
	duringReconfig uint32
}

func NewServer(configFilePath string) (*Server, error) {
	s := &Server{
		configFilePath: configFilePath,
		stopChan:       make(chan struct{}),
	}

	conf, err := jconf.ParseConf(configFilePath)
	if err != nil {
		return nil, err
	}

	err = initLog(conf)
	if err != nil {
		return nil, err
	}

	addr, err := net.ResolveTCPAddr("tcp4",
		fmt.Sprintf("%s:%d",
			conf.GetString("dhcp4.kea-master-ip"),
			conf.GetInt("dhcp4.kea-master-port")))
	if err != nil {
		return nil, err
	}

	listener, err := net.ListenTCP("tcp4", addr)
	if err != nil {
		return nil, err
	}

	s.listener = listener
	s.clients = []*Client{}

	if err := s.reloadConf(); err == nil {
		go s.ioloop()
		return s, nil
	} else {
		return nil, err
	}
}

func initLog(conf *jconf.Config) error {
	logFile := "kea-master.log"
	logLevel := "debug"

	if logConf := conf.GetObject("dhcp4.logging"); logConf != nil {
		if logConf.GetBool("log-enable") {
			dir := logConf.GetString("log-file-dir")
			if dir == "" {
				return fmt.Errorf("log file dir is empty")
			}
			logFile = path.Join(dir, logFile)
			logLevel = logConf.GetString("log-level")
		} else {
			logFile = "/dev/null"
			logLevel = "error"
		}
	}
	util.InitLog(logFile, logLevel)
	return nil
}

func (s *Server) reloadConf() error {
	conf, err := jconf.ParseConf(s.configFilePath)
	if err != nil {
		return err
	}

	s.stopClient()

	if s.allocator == nil {
		allocator, err := CreateAddrAllocator(conf)
		if err != nil {
			return err
		}
		s.allocator = allocator
	} else {
		if err = s.allocator.reloadConf(conf); err != nil {
			return err
		}
	}

	return nil
}

func (s *Server) ioloop() {
	for {
		select {
		case <-s.stopChan:
			goto Stop
		default:
		}

		conn, err := s.listener.AcceptTCP()
		if err != nil {
			continue
		}

		conn.SetLinger(0)
		if atomic.LoadUint32(&s.duringReconfig) > 0 {
			conn.Close()
			continue
		}

		client := newClient(conn, s)
		s.clientsLock.Lock()
		s.clients = append(s.clients, client)
		s.clientsLock.Unlock()
		util.Logger().Info("get conn from %s", client.remoteAddr().String())
	}

Stop:
	s.stopChan <- struct{}{}
}

func (s *Server) stop() {
	s.listener.Close()
	s.stopChan <- struct{}{}
	<-s.stopChan

	s.stopClient()
}

func (s *Server) removeClient(c *Client) {
	s.clientsLock.Lock()
	for i, c_ := range s.clients {
		if c_ == c {
			s.clients = append(s.clients[:i], s.clients[i+1:]...)
			break
		}
	}
	s.clientsLock.Unlock()
}

func (s *Server) stopClient() {
	s.clientsLock.Lock()
	for _, client := range s.clients {
		client.Stop()
	}
	s.clientsLock.Unlock()
}
