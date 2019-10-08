package command

type SqBroadcastTransport struct {
	subscriber Receiver
	topic      string
	sqdAddr    string
}

func newSqBroadcastTransport(sqdAddr, topic string) Transport {
	return &SqBroadcastTransport{
		sqdAddr: sqdAddr,
		topic:   topic,
	}
}

func (t *SqBroadcastTransport) Run(s Service, p Protocol, e *EndPoint) {
	receiver, err := NewReceiver(t.sqdAddr, t.topic, e.Name, e.IP)
	if err != nil {
		panic("connect to sqd failed:" + err.Error())
	}
	t.subscriber = receiver
	t.loop(s, p)
}

func (t *SqBroadcastTransport) loop(s Service, p Protocol) {
	var inCh <-chan []byte
	for {
		inCh = t.subscriber.GetDataChan()

		select {
		case data := <-inCh:
			cmd, err := p.DecodeCmd(data)
			if err == nil {
				s.HandleCmd(cmd)
			}
		}
	}
}
