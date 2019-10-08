package command

import (
	"kea/util"

	"encoding/binary"
	"fmt"
	"io"
	"net"
	"sync"
	"time"
)

const (
	IODeadLine = 3 * time.Second
)

const MaxCmdLen = int16(10240)

type RawTCP struct {
	listener *net.TCPListener
	service  Service
	protocol Protocol

	clients     []*client
	clientsLock sync.Mutex
}

func newRawTCP() Transport {
	return &RawTCP{}
}

type client struct {
	server   *RawTCP
	conn     *net.TCPConn
	stopChan chan struct{}
}

func newClinet(server *RawTCP, conn *net.TCPConn) *client {
	return &client{
		server:   server,
		conn:     conn,
		stopChan: make(chan struct{}),
	}
}

func (c *client) readCommand() (Command, error) {
	var msgSize int16

	c.conn.SetReadDeadline(time.Now().Add(IODeadLine))
	err := binary.Read(c.conn, binary.BigEndian, &msgSize)
	if err != nil {
		return nil, err
	}

	if msgSize <= 0 || msgSize > MaxCmdLen {
		return nil, fmt.Errorf("cmd is too big")
	}

	buf := make([]byte, msgSize)
	c.conn.SetReadDeadline(time.Now().Add(IODeadLine))
	_, err = io.ReadFull(c.conn, buf)
	if err != nil {
		return nil, err
	}

	cmd, err := c.server.protocol.DecodeCmd(buf)
	if cmd == nil {
		cmd = DefaultUnkownCmd
	}
	return cmd, nil
}

func (c *client) ioLoop() {
	c.server.addClient(c)

	getStopCmd := false
	for {
		select {
		case <-c.stopChan:
			c.stopChan <- struct{}{}
			break
		default:
		}

		cmd, err := c.readCommand()
		if err != nil {
			break
		}

		util.Logger().Debug("recv command %s from client %s", cmd.String(), c.conn.RemoteAddr())
		result := c.server.service.HandleCmd(cmd)
		resultData, _ := c.server.protocol.EncodeCmdResult(result)

		c.conn.SetWriteDeadline(time.Now().Add(IODeadLine))
		err = binary.Write(c.conn, binary.BigEndian, uint16(len(resultData)))
		if err != nil {
			break
		}

		c.conn.SetWriteDeadline(time.Now().Add(IODeadLine))
		_, err = c.conn.Write(resultData)
		if err != nil {
			break
		}

		util.Logger().Debug("sent result %v to client %s", result.Succeed, c.conn.RemoteAddr())
		if _, ok := cmd.(*Stop); ok {
			getStopCmd = true
			break
		}
	}

	util.Logger().Info("client %s disconnect with command server", c.conn.RemoteAddr())
	c.conn.Close()
	c.server.removeClient(c)
	if getStopCmd {
		c.server.stop()
	}
}

func (c *client) stop() {
	c.conn.Close()
	c.stopChan <- struct{}{}
	<-c.stopChan
}

func (transport *RawTCP) Run(service Service, protocol Protocol, endpoint *EndPoint) {
	addr, err := net.ResolveTCPAddr("tcp4", fmt.Sprintf("%s:%d", endpoint.IP, endpoint.Port))
	if err != nil {
		panic(err.Error())
	}

	ln, err := net.ListenTCP("tcp4", addr)
	if err != nil {
		panic(err.Error())
	}

	transport.listener = ln
	transport.service = service
	transport.protocol = protocol

	for {
		conn, _ := ln.AcceptTCP()
		if conn != nil {
			util.Logger().Info("client %s connect with command server", conn.RemoteAddr())
			c := newClinet(transport, conn)
			go c.ioLoop()
		} else {
			break
		}
	}
}

func (transport *RawTCP) addClient(c *client) {
	transport.clientsLock.Lock()
	transport.clients = append(transport.clients, c)
	transport.clientsLock.Unlock()
}

func (transport *RawTCP) removeClient(c *client) {
	transport.clientsLock.Lock()
	for i, c_ := range transport.clients {
		if c_ == c {
			transport.clients = append(transport.clients[:i], transport.clients[i+1:]...)
			break
		}
	}
	transport.clientsLock.Unlock()
}

func (transport *RawTCP) stop() {
	transport.listener.Close()
	transport.clientsLock.Lock()
	for _, c := range transport.clients {
		c.stop()
	}
	transport.clientsLock.Unlock()
}
