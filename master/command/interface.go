package command

type Command interface {
	String() string
}

type CmdResult struct {
	Succeed  bool        `json:"succeed"`
	ErrorMsg string      `json:"error_msg"`
	Result   interface{} `json:"result"`
}

type Service interface {
	SupportedCmds() []Command
	HandleCmd(Command) *CmdResult
}

type EndPoint struct {
	IP   string
	Port int
}

type Protocol interface {
	EncodeCmd(Command) ([]byte, error)
	DecodeCmd(data []byte) (Command, error)

	EncodeCmdResult(*CmdResult) ([]byte, error)
	DecodeCmdResult([]byte) (*CmdResult, error)
}

type Transport interface {
	Run(Service, Protocol, *EndPoint)
}
