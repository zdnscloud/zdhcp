package command

type Stop struct {
}

func (c *Stop) String() string {
	return "stop kea master"
}

type UnknownCmd struct {
}

func (c *UnknownCmd) String() string {
	return ""
}

var DefaultUnkownCmd Command = &UnknownCmd{}
