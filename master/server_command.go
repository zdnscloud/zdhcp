package kea

import (
	"fmt"
	"sync/atomic"

	"kea/command"
	"kea/util"
)

type Reconfig struct {
}

func (c *Reconfig) String() string {
	return "reconfig kea master"
}

type GetClientCount struct {
}

func (cnt *GetClientCount) String() string {
	return "get kea master client count"
}

func (s *Server) SupportedCmds() []command.Command {
	return []command.Command{&Reconfig{}, &GetClientCount{}, &AddSubnet{}, &DeleteSubnet{}, &UpdateSubnet{},
		&AddPool{}, &DeletePool{}, &AddSharedNetwork{}, &DeleteSharedNetwork{}, &UpdateSharedNetwork{}}
}

func (s *Server) HandleCmd(cmd command.Command) *command.CmdResult {
	s.cmdExecuteLock.Lock()
	defer s.cmdExecuteLock.Unlock()
	atomic.StoreUint32(&s.duringReconfig, 1)
	defer atomic.StoreUint32(&s.duringReconfig, 0)

	switch c := cmd.(type) {
	case *Reconfig:
		if err := s.reloadConf(); err != nil {
			panic("reload config file failed:" + err.Error())
		}
		return &command.CmdResult{Succeed: true}
	case *GetClientCount:
		s.clientsLock.Lock()
		defer s.clientsLock.Unlock()
		return &command.CmdResult{Succeed: true, Result: len(s.clients)}
	case *command.Stop:
		s.stop()
		return &command.CmdResult{Succeed: true}
	case *AddSubnet:
		if err := s.addSubnet(SubnetID(c.SubnetId), c.Ttl); err != nil {
			util.Logger().Error("add subnet %d failed: %v\n", c.SubnetId, err.Error())
			return &command.CmdResult{Succeed: false}
		} else {
			return &command.CmdResult{Succeed: true}
		}
	case *DeleteSubnet:
		if err := s.deleteSubnet(SubnetID(c.SubnetId)); err != nil {
			util.Logger().Error("delete subnet %d failed: %v\n", c.SubnetId, err.Error())
			return &command.CmdResult{Succeed: false}
		} else {
			return &command.CmdResult{Succeed: true}
		}
	case *UpdateSubnet:
		if err := s.updateSubnet(SubnetID(c.SubnetId), c.Ttl); err != nil {
			util.Logger().Error("update subnet %d failed: %v\n", c.SubnetId, err.Error())
			return &command.CmdResult{Succeed: false}
		} else {
			return &command.CmdResult{Succeed: true}
		}
	case *AddPool:
		if err := s.addPool(SubnetID(c.SubnetId), c.StartIP, c.EndIP, c.HWAddr, c.IPAddr, c.Reserved); err != nil {
			util.Logger().Error("add pool to subnet %d failed: %v\n", c.SubnetId, err.Error())
			return &command.CmdResult{Succeed: false}
		} else {
			return &command.CmdResult{Succeed: true}
		}
	case *DeletePool:
		if err := s.deletePool(SubnetID(c.SubnetId), c.StartIP, c.EndIP, c.HWAddr, c.IPAddr, c.Reserved); err != nil {
			util.Logger().Error("delete pool from subnet %d failed: %v\n", c.SubnetId, err.Error())
			return &command.CmdResult{Succeed: false}
		} else {
			return &command.CmdResult{Succeed: true}
		}
	case *AddSharedNetwork:
		if err := s.addSharedNetwork(c.Name, c.SubnetIds); err != nil {
			util.Logger().Error("add shared network %s failed: %v\n", c.Name, err.Error())
			return &command.CmdResult{Succeed: false}
		} else {
			return &command.CmdResult{Succeed: true}
		}
	case *DeleteSharedNetwork:
		if err := s.deleteSharedNetwork(c.Name); err != nil {
			util.Logger().Error("delete shared network %s failed: %v\n", c.Name, err.Error())
			return &command.CmdResult{Succeed: false}
		} else {
			return &command.CmdResult{Succeed: true}
		}
	case *UpdateSharedNetwork:
		if err := s.updateSharedNetwork(c.Name, c.SubnetIds); err != nil {
			util.Logger().Error("update shared network %s failed: %v\n", c.Name, err.Error())
			return &command.CmdResult{Succeed: false}
		} else {
			return &command.CmdResult{Succeed: true}
		}
	default:
		return &command.CmdResult{
			Succeed:  false,
			ErrorMsg: fmt.Sprintf("unknown cmd %v", cmd),
		}
	}
}
