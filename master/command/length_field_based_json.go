package command

import (
	"cement/serializer"

	"encoding/json"
)

type LengthFieldBasedJson struct {
	serializer *serializer.Serializer
}

func newLengthFieldBasedJson(cmds []Command) (Protocol, error) {
	s := serializer.NewSerializer()
	for _, c := range cmds {
		if err := s.Register(c); err != nil {
			return nil, err
		}
	}
	s.Register(&Stop{})

	return &LengthFieldBasedJson{
		serializer: s,
	}, nil
}

func (p *LengthFieldBasedJson) DecodeCmd(data []byte) (Command, error) {
	cmd, err := p.serializer.Decode(data)
	if err == nil {
		return cmd.(Command), nil
	} else {
		return nil, err
	}
}

func (p *LengthFieldBasedJson) EncodeCmd(c Command) ([]byte, error) {
	return p.serializer.Encode(c)
}

func (p *LengthFieldBasedJson) EncodeCmdResult(result *CmdResult) ([]byte, error) {
	return json.Marshal(result)
}

func (p *LengthFieldBasedJson) DecodeCmdResult(data []byte) (*CmdResult, error) {
	var result CmdResult
	err := json.Unmarshal(data, &result)
	if err == nil {
		return &result, err
	} else {
		return nil, err
	}
}
