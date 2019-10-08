package command

import ()

type SqMaster struct {
	protocol  Protocol
	publisher Sender
	inputCh   chan Command
	cmds      []Command
}

func newSqMaster(sqdAddr, localAddr, topic string, p Protocol, cmds []Command) (*SqMaster, error) {
	sender, err := NewSender(sqdAddr, topic, localAddr)
	if err != nil {
		return nil, err
	}

	proxy := &SqMaster{
		protocol:  p,
		publisher: sender,
		inputCh:   make(chan Command),
	}

	go proxy.loop()
	return proxy, nil
}

func (p *SqMaster) SupportedCmds() []Command {
	return p.cmds
}

func (p *SqMaster) HandleCmd(c Command) *CmdResult {
	p.inputCh <- c
	return &CmdResult{Succeed: true}
}

func (p *SqMaster) loop() {
	inCh := p.inputCh
	outCh := p.publisher.GetDataChan()

	for {
		cmd := <-inCh
		data, _ := p.protocol.EncodeCmd(cmd)
		outCh <- data
	}
}
