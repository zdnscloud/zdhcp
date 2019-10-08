package command

func RunBroadcastSlave(s Service, sqdAddr, topic string, e *EndPoint) {
	p, err := newLengthFieldBasedJson(s.SupportedCmds())
	if err != nil {
		panic(err.Error())
	}

	newSqBroadcastTransport(sqdAddr, topic).Run(s, p, e)
}

func NewBroadcastMaster(sqdAddr, localAddr, topic string, cmds []Command) (Service, error) {
	p, err := newLengthFieldBasedJson(cmds)
	if err != nil {
		return nil, err
	}

	return newSqMaster(sqdAddr, localAddr, topic, p, cmds)
}
