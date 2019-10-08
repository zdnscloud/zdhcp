package kea

import (
	"net"
	"sync"
	"time"

	"cement/jconf"
	"kea/util"
)

type LeaseOptType int

const (
	OptAddLease     LeaseOptType = 0
	OptDeleteLease               = 1
	OptUpdateLease               = 2
	OptLoadLease                 = 3
	OptDeleteSubnet              = 4
	OptDeletePool                = 5
)

const OptChanBufferLen int = 1024
const DefaultDBConnCount int = 8
const OptBufCount int = 40
const MaxDBDelay time.Duration = 10 * time.Second

type LeaseOpt struct {
	typ   LeaseOptType
	param interface{}
}

type deleteLeaseTask struct {
	addr     net.IP
	subnetId SubnetID
	startIp  uint32
	endIp    uint32
}

var _leaseDBPoolInstance *LeaseDBPool

type LeaseDBPool struct {
	dbCount         int
	dbs             []*LeaseDB
	optChans        []chan LeaseOpt
	stopChan        chan int
	subnetToDbIndex map[SubnetID]int
}

func initLeaseDBPool(conf *jconf.Config) error {
	instance, err := newLeaseDBPool(DefaultDBConnCount, conf.GetString("dhcp4.lease-database.host"),
		conf.GetString("dhcp4.lease-database.user"),
		conf.GetString("dhcp4.lease-database.password"),
		conf.GetString("dhcp4.lease-database.name"))
	if err != nil {
		return err
	}

	_leaseDBPoolInstance = instance
	return nil
}

func LeaseDBPoolInstance() *LeaseDBPool {
	return _leaseDBPoolInstance
}

func newLeaseDBPool(count int, host, user, passwd, dbname string) (*LeaseDBPool, error) {
	dbs := []*LeaseDB{}
	chans := []chan LeaseOpt{}
	util.Logger().Info("create %d conn to db", count)
	for i := 0; i < count; i++ {
		db, err := NewLeaseDB(host, user, passwd, dbname)
		if err != nil {
			return nil, err
		}
		dbs = append(dbs, db.(*LeaseDB))
		chans = append(chans, make(chan LeaseOpt, OptChanBufferLen))
	}

	dbPool := &LeaseDBPool{
		dbCount:         count,
		dbs:             dbs,
		optChans:        chans,
		stopChan:        make(chan int),
		subnetToDbIndex: make(map[SubnetID]int),
	}

	go dbPool.run()
	return dbPool, nil
}

func (pool *LeaseDBPool) run() {
	for dbIndex := 0; dbIndex < pool.dbCount; dbIndex++ {
		go pool.runDB(pool.dbs[dbIndex], pool.optChans[dbIndex])
	}
}

func (pool *LeaseDBPool) runDB(db *LeaseDB, optChan chan LeaseOpt) {
	flusher := time.NewTicker(MaxDBDelay)
	optBuf := make([]LeaseOpt, 0, OptBufCount)
	flushDBOp := func() {
		if len(optBuf) == 0 {
			return
		}

		tx, err := db.Begin()
		if err != nil {
			panic("db connect failed: " + err.Error())
		}
		for _, opt := range optBuf {
			switch opt.typ {
			case OptAddLease:
				db.AddLeaseInTx(tx, opt.param.(*Lease))
			case OptUpdateLease:
				db.UpdateLeaseInTx(tx, opt.param.(*Lease))
			case OptDeleteLease:
				task := opt.param.(*deleteLeaseTask)
				db.DeleteLeaseInTx(tx, task.subnetId, task.addr)
			case OptDeleteSubnet:
				task := opt.param.(*deleteLeaseTask)
				db.DeleteSubnetInTx(tx, task.subnetId)
			case OptDeletePool:
				task := opt.param.(*deleteLeaseTask)
				db.DeletePoolInTx(tx, task.subnetId, task.startIp, task.endIp)
			}
		}
		tx.Commit()
		optBuf = optBuf[:0]
	}

	for {
		select {
		case opt, ok := <-optChan:
			if ok == false {
				flushDBOp()
				goto Done
			}
			if opt.typ == OptLoadLease {
				flushDBOp()
				db.LoadLease(opt.param.(*LoadLeaseTask))
			} else {
				optBuf = append(optBuf, opt)
				if len(optBuf) == OptBufCount {
					flushDBOp()
				}
			}
		case <-flusher.C:
			flushDBOp()
		}
	}

Done:
	pool.stopChan <- 0
}

func (pool *LeaseDBPool) AddLease(lease *Lease) {
	pool.asyncDoOption(lease.SubnetId, LeaseOpt{
		typ:   OptAddLease,
		param: lease,
	})
}

func (pool *LeaseDBPool) UpdateLease(lease *Lease) {
	pool.asyncDoOption(lease.SubnetId, LeaseOpt{
		typ:   OptUpdateLease,
		param: lease,
	})
}

func (pool *LeaseDBPool) DeleteLease(subnetId SubnetID, addr net.IP) {
	pool.asyncDoOption(subnetId, LeaseOpt{
		typ: OptDeleteLease,
		param: &deleteLeaseTask{
			addr:     addr,
			subnetId: subnetId,
		},
	})
}

func (pool *LeaseDBPool) asyncDoOption(subnetId SubnetID, opt LeaseOpt) {
	if targetDBIndex, ok := pool.subnetToDbIndex[subnetId]; ok {
		pool.optChans[targetDBIndex] <- opt
	} else {
		util.Logger().Error("unknown subnet with id %d", subnetId)
	}
}

func (pool *LeaseDBPool) Stop() {
	for _, ch := range pool.optChans {
		close(ch)
	}

	for i := 0; i < pool.dbCount; i++ {
		<-pool.stopChan
	}

	for _, db := range pool.dbs {
		db.Close()
	}
}

func (pool *LeaseDBPool) LoadLease(subnetAndLeaseManager map[*Subnet]LeaseManager) {
	oldSubnets := make(map[*Subnet]LeaseManager)
	newSubnets := make(map[*Subnet]LeaseManager)
	for subnet, manager := range subnetAndLeaseManager {
		if _, ok := pool.subnetToDbIndex[subnet.Id]; ok {
			oldSubnets[subnet] = manager
		} else {
			newSubnets[subnet] = manager
		}
	}
	pool.loadLeaseForSubnets(oldSubnets)
	pool.allocateDBConn(oldSubnets, newSubnets)
	if len(newSubnets) > 0 {
		pool.loadLeaseForSubnets(newSubnets)
	}
}

func (pool *LeaseDBPool) loadLeaseForSubnets(subnets map[*Subnet]LeaseManager) {
	var wg sync.WaitGroup
	for subnet, manager := range subnets {
		wg.Add(1)
		go func(subnet *Subnet, manager LeaseManager) {
			notifyChan := make(chan struct{})
			pool.asyncDoOption(subnet.Id,
				LeaseOpt{
					typ: OptLoadLease,
					param: &LoadLeaseTask{
						notifyChan:   notifyChan,
						subnet:       subnet,
						leaseManager: manager,
					},
				})
			<-notifyChan
			wg.Done()
		}(subnet, manager)
	}
	wg.Wait()
}

func (pool *LeaseDBPool) allocateDBConn(oldSubnets map[*Subnet]LeaseManager, newSubnets map[*Subnet]LeaseManager) {
	subnetToDbIndex := make(map[SubnetID]int)
	oldSubnetToDbIndex := pool.subnetToDbIndex
	dbIndex := 0

	for subnet, _ := range oldSubnets {
		delete(oldSubnetToDbIndex, subnet.Id)
		subnetToDbIndex[subnet.Id] = dbIndex
		dbIndex = (dbIndex + 1) % pool.dbCount
	}

	for subnet, _ := range newSubnets {
		subnetToDbIndex[subnet.Id] = dbIndex
		dbIndex = (dbIndex + 1) % pool.dbCount
	}

	for removeSubnetId, _ := range oldSubnetToDbIndex {
		util.Logger().Debug("Invalid subnet id %v and will remove it from db", removeSubnetId)
		pool.DeleteLease(removeSubnetId, nil)
	}

	pool.subnetToDbIndex = subnetToDbIndex
}
