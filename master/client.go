package kea

import (
	"encoding/binary"
	"github.com/golang/protobuf/proto"
	"io"
	"net"

	"kea/util"
)

type Client struct {
	conn     *net.TCPConn
	server   *Server
	stopChan chan struct{}
}

func newClient(conn *net.TCPConn, server *Server) *Client {
	c := &Client{
		conn:     conn,
		server:   server,
		stopChan: make(chan struct{}, 1),
	}
	go c.ioloop()
	return c
}

func (c *Client) ioloop() {
	for {
		ctx, err := c.readContext()
		if err != nil {
			util.Logger().Error("read query failed %s", err.Error())
			goto Disconnect
		}

		result := c.server.allocator.HandleRequest(ctx)
		resultData, _ := proto.Marshal(&result)
		err = binary.Write(c.conn, binary.BigEndian, uint16(len(resultData)))
		if err != nil {
			util.Logger().Error("write get error %s", err.Error())
			goto Disconnect
		}

		_, err = c.conn.Write(resultData)
		if err != nil {
			util.Logger().Error("write get error %s", err.Error())
			goto Disconnect
		}
	}

Disconnect:
	util.Logger().Info("disconnect with client %s", c.remoteAddr().String())
	c.stopChan <- struct{}{}
	c.server.removeClient(c)
}

func (c *Client) Stop() {
	c.conn.Close()
	<-c.stopChan
}

func (c *Client) remoteAddr() net.Addr {
	return c.conn.RemoteAddr()
}

func (c *Client) readContext() (*Context, error) {
	var msgSize int16

	err := binary.Read(c.conn, binary.BigEndian, &msgSize)
	if err != nil {
		return nil, err
	}

	buf := make([]byte, msgSize)
	_, err = io.ReadFull(c.conn, buf)
	if err != nil {
		return nil, err
	}

	msg := &ContextMsg{}
	err = proto.Unmarshal(buf, msg)
	if err != nil {
		return nil, err
	}

	return FromContextMsg(msg), nil
}
