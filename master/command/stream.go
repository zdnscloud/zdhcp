package command

type Receiver interface {
	GetDataChan() <-chan []byte
}

type Sender interface {
	GetDataChan() chan<- []byte
}
