package command

func RunTcpCmdServer(s Service, e *EndPoint) {
	p, err := newLengthFieldBasedJson(s.SupportedCmds())
	if err != nil {
		panic(err.Error())
	}

	newRawTCP().Run(s, p, e)
}
